/*
 * lock.c:  routines for locking working copy subdirectories.
 *
 * ====================================================================
 * Copyright (c) 2000-2008 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

#include <apr_pools.h>
#include <apr_time.h>

#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_sorts.h"
#include "svn_types.h"

#include "wc.h"
#include "adm_files.h"
#include "lock.h"
#include "questions.h"
#include "props.h"
#include "log.h"
#include "entries.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"



typedef struct
{
  /* SET is a hash of svn_wc_adm_access_t* keyed on char* representing the
     path to directories that are open. */
  apr_hash_t *set;

} svn_wc__adm_shared_t;


struct svn_wc_adm_access_t
{
  /* The working copy format version number for the directory */
  int wc_format;

  /* PATH to directory which contains the administrative area */
  const char *path;

  enum svn_wc__adm_access_type {

    /* SVN_WC__ADM_ACCESS_UNLOCKED indicates no lock is held allowing
       read-only access */
    svn_wc__adm_access_unlocked,

    /* SVN_WC__ADM_ACCESS_WRITE_LOCK indicates that a write lock is held
       allowing read-write access */
    svn_wc__adm_access_write_lock,

    /* SVN_WC__ADM_ACCESS_CLOSED indicates that the baton has been
       closed. */
    svn_wc__adm_access_closed

  } type;

  /* LOCK_EXISTS is set TRUE when the write lock exists */
  svn_boolean_t lock_exists;

  /* SHARED contains state that is shared among all associated
     access batons. */
  svn_wc__adm_shared_t *shared;

  /* SET_OWNER is TRUE if SET is allocated from this access baton */
  svn_boolean_t set_owner;

  /* ENTRIES is the cached entries for PATH, without those in state
     deleted. ENTRIES_HIDDEN is the cached entries including those in
     state deleted or state absent. Either may be NULL. */
  apr_hash_t *entries;
  apr_hash_t *entries_hidden;

  /* A hash mapping const char * entry names to hashes of wcprops.
     These hashes map const char * names to svn_string_t * values.
     NULL of the wcprops hasn't been read into memory.
     ### Since there are typically just one or two wcprops per entry,
     ### we could use a more compact way of storing them. */
  apr_hash_t *wcprops;

  /* POOL is used to allocate cached items, they need to persist for the
     lifetime of this access baton */
  apr_pool_t *pool;

};

/* This is a placeholder used in the set hash to represent missing
   directories.  Only its address is important, it contains no useful
   data. */
static const svn_wc_adm_access_t missing;


static svn_error_t *
do_close(svn_wc_adm_access_t *adm_access, svn_boolean_t preserve_lock,
         svn_boolean_t recurse, apr_pool_t *scratch_pool);


/* Defining this conditional will result in a client that will refuse to
   upgrade working copies.  This can be useful if you want to avoid
   problems caused by accidentally running a development version of SVN
   on a working copy that you typically use with an older version. */
#ifndef SVN_DISABLE_WC_UPGRADE

/* Write, to LOG_ACCUM, log entries to convert an old WC that did not have
   propcaching into a WC that uses propcaching.  Do this conversion for
   the directory of ADM_ACCESS and its file children.  Use POOL for
   temporary allocations.  */
static svn_error_t *
introduce_propcaching(svn_stringbuf_t *log_accum,
                      svn_wc_adm_access_t *adm_access,
                      apr_pool_t *pool)
{
  apr_hash_t *entries;
  apr_hash_index_t *hi;
  apr_pool_t *subpool = svn_pool_create(pool);

  SVN_ERR(svn_wc_entries_read(&entries, adm_access, FALSE, pool));

  /* Reinstall the properties for each file and this dir; subdirs are handled
     when they're opened. */
  for (hi = apr_hash_first(pool, entries); hi; hi = apr_hash_next(hi))
    {
      void *val;
      const svn_wc_entry_t *entry;
      const char *entrypath;
      apr_hash_t *base_props, *props;

      apr_hash_this(hi, NULL, NULL, &val);
      entry = val;

      if (entry->kind != svn_node_file
          && strcmp(entry->name, SVN_WC_ENTRY_THIS_DIR) != 0)
        continue;

      svn_pool_clear(subpool);

      entrypath = svn_path_join(adm_access->path, entry->name, subpool);
      SVN_ERR(svn_wc__load_props(&base_props, &props, NULL, adm_access,
                                 entrypath, subpool));
      SVN_ERR(svn_wc__install_props(&log_accum, adm_access, entrypath,
                                    base_props, props, TRUE, subpool));
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

/* Write, to LOG_ACCUM, commands to convert a WC that has wcprops in individual
   files to use one wcprops file per directory.
   Do this for ADM_ACCESS and its file children, using POOL for temporary
   allocations. */
static svn_error_t *
convert_wcprops(svn_stringbuf_t *log_accum,
                svn_wc_adm_access_t *adm_access,
                apr_pool_t *pool)
{
  apr_hash_t *entries;
  apr_hash_index_t *hi;
  apr_pool_t *subpool = svn_pool_create(pool);

  SVN_ERR(svn_wc_entries_read(&entries, adm_access, FALSE, pool));

  /* Walk over the entries, adding a modify-wcprop command for each wcprop.
     Note that the modifications happen in memory and are just written once
     at the end of the log execution, so this isn't as inefficient as it
     might sound. */
  for (hi = apr_hash_first(pool, entries); hi; hi = apr_hash_next(hi))
    {
      void *val;
      const svn_wc_entry_t *entry;
      apr_hash_t *wcprops;
      apr_hash_index_t *hj;
      const char *full_path;

      apr_hash_this(hi, NULL, NULL, &val);
      entry = val;

      full_path = svn_path_join(adm_access->path, entry->name, pool);

      if (entry->kind != svn_node_file
          && strcmp(entry->name, SVN_WC_ENTRY_THIS_DIR) != 0)
        continue;

      svn_pool_clear(subpool);

      SVN_ERR(svn_wc__wcprop_list(&wcprops, entry->name, adm_access, subpool));

      /* Create a subsubpool for the inner loop...
         No, just kidding.  There are typically just one or two wcprops
         per entry... */
      for (hj = apr_hash_first(subpool, wcprops); hj; hj = apr_hash_next(hj))
        {
          const void *key2;
          void *val2;
          const char *propname;
          svn_string_t *propval;

          apr_hash_this(hj, &key2, NULL, &val2);
          propname = key2;
          propval = val2;
          SVN_ERR(svn_wc__loggy_modify_wcprop(&log_accum, adm_access,
                                              full_path, propname,
                                              propval->data,
                                              subpool));
        }
    }

  return SVN_NO_ERROR;
}

/* Maybe upgrade the working copy directory represented by ADM_ACCESS
   to the latest 'SVN_WC__VERSION'.  ADM_ACCESS must contain a write
   lock.  Use POOL for all temporary allocation.

   Not all upgrade paths are necessarily supported.  For example,
   upgrading a version 1 working copy results in an error.

   Sometimes the format file can contain "0" while the administrative
   directory is being constructed; calling this on a format 0 working
   copy has no effect and returns no error. */
static svn_error_t *
maybe_upgrade_format(svn_wc_adm_access_t *adm_access, apr_pool_t *pool)
{
  SVN_ERR(svn_wc__check_format(adm_access->wc_format,
                               adm_access->path,
                               pool));

  /* We can upgrade all formats that are accepted by
     svn_wc__check_format. */
  if (adm_access->wc_format != SVN_WC__VERSION)
    {
      svn_boolean_t cleanup_required;
      svn_stringbuf_t *log_accum = svn_stringbuf_create("", pool);

      /* Don't try to mess with the WC if there are old log files left. */
      SVN_ERR(svn_wc__adm_is_cleanup_required(&cleanup_required,
                                              adm_access, pool));
      if (cleanup_required)
        return SVN_NO_ERROR;

      /* First, loggily upgrade the format file. */
      SVN_ERR(svn_wc__loggy_upgrade_format(&log_accum, adm_access,
                                           SVN_WC__VERSION, pool));

      /* Possibly convert an old WC that doesn't use propcaching. */
      if (adm_access->wc_format <= SVN_WC__NO_PROPCACHING_VERSION)
        SVN_ERR(introduce_propcaching(log_accum, adm_access, pool));

      /* If the WC uses one file per entry for wcprops, give back some inodes
         to the poor user. */
      if (adm_access->wc_format <= SVN_WC__WCPROPS_MANY_FILES_VERSION)
        SVN_ERR(convert_wcprops(log_accum, adm_access, pool));

      SVN_ERR(svn_wc__write_log(adm_access, 0, log_accum, pool));

      if (adm_access->wc_format <= SVN_WC__WCPROPS_MANY_FILES_VERSION)
        {
          /* Remove wcprops directory, dir-props, README.txt and empty-file
             files.
             We just silently ignore errors, because keeping these files is
             not catastrophic. */

          svn_error_clear(svn_io_remove_dir2(
              svn_wc__adm_child(adm_access->path, SVN_WC__ADM_WCPROPS, pool),
              FALSE, NULL, NULL, pool));
          svn_error_clear(svn_io_remove_file(
              svn_wc__adm_child(adm_access->path, SVN_WC__ADM_DIR_WCPROPS,
                                pool),
              pool));
          svn_error_clear(svn_io_remove_file(
              svn_wc__adm_child(adm_access->path, SVN_WC__ADM_EMPTY_FILE, pool),
              pool));
          svn_error_clear(svn_io_remove_file(
              svn_wc__adm_child(adm_access->path, SVN_WC__ADM_README, pool),
              pool));
        }

      SVN_ERR(svn_wc__run_log(adm_access, NULL, pool));
    }

  return SVN_NO_ERROR;
}

#else

/* Alternate version of the above for use when working copy upgrades
   are disabled.  Return an error if the working copy described by
   ADM_ACCESS is not at the latest 'SVN_WC__VERSION'.  Use POOL for all
   temporary allocation.  */
static svn_error_t *
maybe_upgrade_format(svn_wc_adm_access_t *adm_access, apr_pool_t *pool)
{
  SVN_ERR(svn_wc__check_format(adm_access->wc_format,
                               adm_access->path,
                               pool));

  if (adm_access->wc_format != SVN_WC__VERSION)
    {
      return svn_error_createf(SVN_ERR_WC_UNSUPPORTED_FORMAT, NULL,
                               "Would upgrade working copy '%s' from old "
                               "format (%d) to current format (%d), "
                               "but automatic upgrade has been disabled",
                               svn_path_local_style(adm_access->path, pool),
                               adm_access->wc_format, SVN_WC__VERSION);
    }

  return SVN_NO_ERROR;
}

#endif


/* Create a physical lock file in the admin directory for ADM_ACCESS. Wait
   up to WAIT_FOR seconds if the lock already exists retrying every
   second.

   Note: most callers of this function determine the wc_format for the
   lock soon afterwards.  We recommend calling maybe_upgrade_format()
   as soon as you have the wc_format for a lock, since that's a good
   opportunity to drag old working directories into the modern era. */
static svn_error_t *
create_lock(const char *path, int wait_for, apr_pool_t *pool)
{
  const char *lock_path = svn_wc__adm_child(path, SVN_WC__ADM_LOCK, pool);
  svn_error_t *err;

  for (;;)
    {
      apr_file_t *file;

      err = svn_io_file_open(&file, lock_path,
                             APR_WRITE | APR_CREATE | APR_EXCL,
                             APR_OS_DEFAULT,
                             pool);
      if (err == NULL)
        return svn_io_file_close(file, pool);

      if (APR_STATUS_IS_EEXIST(err->apr_err))
        {
          svn_error_clear(err);
          if (wait_for <= 0)
            break;
          wait_for--;
          apr_sleep(apr_time_from_sec(1));  /* micro-seconds */
        }
      else
        return err;
    }

  return svn_error_createf(SVN_ERR_WC_LOCKED, NULL,
                           _("Working copy '%s' locked"),
                           svn_path_local_style(path, pool));
}


/* An APR pool cleanup handler.  This handles access batons that have not
   been closed when their pool gets destroyed.  The physical locks
   associated with such batons remain in the working copy if they are
   protecting a log file. */
static apr_status_t
pool_cleanup(void *p)
{
  svn_wc_adm_access_t *lock = p;
  svn_boolean_t cleanup;
  svn_error_t *err;

  if (lock->type == svn_wc__adm_access_closed)
    return SVN_NO_ERROR;

  err = svn_wc__adm_is_cleanup_required(&cleanup, lock, lock->pool);
  if (!err)
    err = do_close(lock, cleanup, TRUE, lock->pool);

  /* ### Is this the correct way to handle the error? */
  if (err)
    {
      apr_status_t apr_err = err->apr_err;
      svn_error_clear(err);
      return apr_err;
    }
  else
    return APR_SUCCESS;
}

/* An APR pool cleanup handler.  This is a child handler, it removes the
   main pool handler. */
static apr_status_t
pool_cleanup_child(void *p)
{
  svn_wc_adm_access_t *lock = p;
  apr_pool_cleanup_kill(lock->pool, lock, pool_cleanup);
  return APR_SUCCESS;
}

/* Allocate from POOL, initialise and return an access baton. TYPE and PATH
   are used to initialise the baton.  */
static svn_wc_adm_access_t *
adm_access_alloc(enum svn_wc__adm_access_type type,
                 const char *path,
                 apr_pool_t *pool)
{
  svn_wc_adm_access_t *lock = apr_palloc(pool, sizeof(*lock));
  lock->type = type;
  lock->entries = NULL;
  lock->entries_hidden = NULL;
  lock->wcprops = NULL;
  lock->wc_format = 0;
  lock->shared = NULL;
  lock->lock_exists = FALSE;
  lock->set_owner = FALSE;
  lock->path = apr_pstrdup(pool, path);
  lock->pool = pool;

  return lock;
}

static void
adm_ensure_set(svn_wc_adm_access_t *adm_access)
{
  if (adm_access->shared == NULL)
    adm_access->shared = apr_pcalloc(adm_access->pool,
                                     sizeof(*adm_access->shared));

  if (adm_access->shared->set == NULL)
    {
      adm_access->set_owner = TRUE;
      adm_access->shared->set = apr_hash_make(adm_access->pool);
      apr_hash_set(adm_access->shared->set,
                   adm_access->path, APR_HASH_KEY_STRING,
                   adm_access);
    }
}

static svn_error_t *
probe(const char **dir,
      const char *path,
      int *wc_format,
      apr_pool_t *pool)
{
  svn_node_kind_t kind;

  SVN_ERR(svn_io_check_path(path, &kind, pool));
  if (kind == svn_node_dir)
    SVN_ERR(svn_wc_check_wc(path, wc_format, pool));
  else
    *wc_format = 0;

  /* a "version" of 0 means a non-wc directory */
  if (kind != svn_node_dir || *wc_format == 0)
    {
      /* Passing a path ending in "." or ".." to svn_path_dirname() is
         probably always a bad idea; certainly it is in this case.
         Unfortunately, svn_path_dirname()'s current signature can't
         return an error, so we have to insert the protection in this
         caller, as making the larger API change would be very
         destabilizing right now (just before 1.0).  See issue #1617. */
      const char *base_name = svn_path_basename(path, pool);
      if ((strcmp(base_name, "..") == 0)
          || (strcmp(base_name, ".") == 0))
        {
          return svn_error_createf
            (SVN_ERR_WC_BAD_PATH, NULL,
             _("Path '%s' ends in '%s', "
               "which is unsupported for this operation"),
             svn_path_local_style(path, pool), base_name);
        }

      *dir = svn_path_dirname(path, pool);
    }
  else
    *dir = path;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__adm_steal_write_lock(svn_wc_adm_access_t **adm_access,
                             svn_wc_adm_access_t *associated,
                             const char *path,
                             apr_pool_t *pool)
{
  svn_error_t *err;
  svn_wc_adm_access_t *lock = adm_access_alloc(svn_wc__adm_access_write_lock,
                                               path, pool);

  err = create_lock(path, 0, pool);
  if (err)
    {
      if (err->apr_err == SVN_ERR_WC_LOCKED)
        svn_error_clear(err);  /* Steal existing lock */
      else
        return err;
    }

  if (associated)
    {
      adm_ensure_set(associated);
      lock->shared = associated->shared;
      apr_hash_set(lock->shared->set, lock->path, APR_HASH_KEY_STRING, lock);
    }

  /* We have a write lock.  If the working copy has an old
     format, this is the time to upgrade it. */
  SVN_ERR(svn_wc_check_wc(path, &lock->wc_format, pool));
  SVN_ERR(maybe_upgrade_format(lock, pool));

  lock->lock_exists = TRUE;
  *adm_access = lock;
  return SVN_NO_ERROR;
}

/* This is essentially the guts of svn_wc_adm_open3, with the additional
 * parameter UNDER_CONSTRUCTION that gets set TRUE only when locking the
 * admin directory during initial creation.
 *
 * If the working copy is already locked, return SVN_ERR_WC_LOCKED; if
 * it is not a versioned directory, return SVN_ERR_WC_NOT_DIRECTORY.
 */
static svn_error_t *
do_open(svn_wc_adm_access_t **adm_access,
        svn_wc__adm_shared_t *shared,
        const char *path,
        svn_boolean_t write_lock,
        int levels_to_lock,
        svn_boolean_t under_construction,
        svn_cancel_func_t cancel_func,
        void *cancel_baton,
        apr_pool_t *pool)
{
  svn_wc_adm_access_t *lock;
  int wc_format;
  svn_error_t *err;
  apr_pool_t *subpool = svn_pool_create(pool);

  if (shared)
    {
      lock = apr_hash_get(shared->set, path, APR_HASH_KEY_STRING);
      if (lock && lock != &missing)
        /* Already locked.  The reason we don't return the existing baton
           here is that the user is supposed to know whether a directory is
           locked: if it's not locked call svn_wc_adm_open, if it is locked
           call svn_wc_adm_retrieve.  */
        return svn_error_createf(SVN_ERR_WC_LOCKED, NULL,
                                 _("Working copy '%s' locked"),
                                 svn_path_local_style(path, pool));
    }

  if (! under_construction)
    {
      err = svn_wc_check_wc(path, &wc_format, subpool);

      if (wc_format == 0 || (err && APR_STATUS_IS_ENOENT(err->apr_err)))
        {
          return svn_error_createf(SVN_ERR_WC_NOT_DIRECTORY, err,
                                   _("'%s' is not a working copy"),
                                   svn_path_local_style(path, pool));
        }
    }

  /* Need to create a new lock */
  if (write_lock)
    {
      lock = adm_access_alloc(svn_wc__adm_access_write_lock, path, pool);
      SVN_ERR(create_lock(path, 0, subpool));
      lock->lock_exists = TRUE;
    }
  else
    {
      lock = adm_access_alloc(svn_wc__adm_access_unlocked, path, pool);
    }

  if (! under_construction)
    {
      lock->wc_format = wc_format;
      if (write_lock)
        SVN_ERR(maybe_upgrade_format(lock, subpool));
    }

  if (levels_to_lock != 0)
    {
      svn_wc__adm_shared_t fake_shared = { 0 };
      svn_wc__adm_shared_t *lock_shared;
      apr_hash_t *entries;
      apr_hash_index_t *hi;

      /* Reduce levels_to_lock since we are about to recurse */
      if (levels_to_lock > 0)
        levels_to_lock--;

      SVN_ERR(svn_wc_entries_read(&entries, lock, FALSE, subpool));

      if (apr_hash_count(entries) > 0)
        {
          if (shared)
            {
              /* Use a temporary hash until all children have been opened. */
              fake_shared.set = apr_hash_make(subpool);
              lock_shared = &fake_shared;
            }
          else
            {
              /* All the batons will accumulate on <lock>. */
              adm_ensure_set(lock);
              lock_shared = lock->shared;
            }
        }

      /* Open the tree */
      for (hi = apr_hash_first(subpool, entries); hi; hi = apr_hash_next(hi))
        {
          void *val;
          const svn_wc_entry_t *entry;
          svn_wc_adm_access_t *entry_access;
          const char *entry_path;

          /* See if someone wants to cancel this operation. */
          if (cancel_func)
            {
              err =  cancel_func(cancel_baton);
              if (err)
                {
                  /* lock->shared->set may have batons in it, or we created
                     a fake set of shared data. Wherever those batons were
                     placed, we need to get them closed. We can fake out
                     do_close() by putting the right "shared" reference into
                     <lock> before closing it. */
                  lock->shared = lock_shared;
                  svn_error_clear(svn_wc_adm_close2(lock, subpool));
                  svn_pool_destroy(subpool);
                  return err;
                }
            }

          apr_hash_this(hi, NULL, NULL, &val);
          entry = val;
          if (entry->kind != svn_node_dir
              || ! strcmp(entry->name, SVN_WC_ENTRY_THIS_DIR))
            continue;

          /* Also skip the excluded subdir. */
          if (entry->depth == svn_depth_exclude)
            continue;

          entry_path = svn_path_join(path, entry->name, subpool);

          /* Don't use the subpool pool here, the lock needs to persist */
          err = do_open(&entry_access, lock_shared, entry_path, write_lock,
                        levels_to_lock, FALSE, cancel_func, cancel_baton,
                        lock->pool);
          if (err)
            {
              if (err->apr_err != SVN_ERR_WC_NOT_DIRECTORY)
                {
                  /* See comment above regarding this assignment. */
                  lock->shared = lock_shared;
                  svn_error_clear(svn_wc_adm_close2(lock, subpool));
                  svn_pool_destroy(subpool);
                  return err;
                }

              /* It's missing or obstructed, so store a placeholder */
              svn_error_clear(err);
              apr_hash_set(lock_shared->set,
                           apr_pstrdup(lock->pool, entry_path),
                           APR_HASH_KEY_STRING, &missing);

              continue;
            }

          /* ### Perhaps we should verify that the parent and child agree
             ### about the URL of the child? */
        }

      /* Switch from temporary hash to permanent hash */
      if (fake_shared.set)
        {
          for (hi = apr_hash_first(subpool, fake_shared.set);
               hi;
               hi = apr_hash_next(hi))
            {
              const void *key;
              void *val;
              const char *entry_path;
              svn_wc_adm_access_t *entry_access;

              apr_hash_this(hi, &key, NULL, &val);
              entry_path = key;
              entry_access = val;
              apr_hash_set(shared->set, entry_path, APR_HASH_KEY_STRING,
                           entry_access);

              if (entry_access == &missing)
                {
                  /* Entry is missing or obstructed; see above */
                  continue; /* Skip or we will write read only memory */
                }

              entry_access->shared = shared;
            }
          lock->shared = shared;
        }
    }

  if (shared)
    {
      lock->shared = shared;
      apr_hash_set(shared->set, lock->path, APR_HASH_KEY_STRING, lock);
    }

  /* It's important that the cleanup handler is registered *after* at least
     one UTF8 conversion has been done, since such a conversion may create
     the apr_xlate_t object in the pool, and that object must be around
     when the cleanup handler runs.  If the apr_xlate_t cleanup handler
     were to run *before* the access baton cleanup handler, then the access
     baton's handler won't work. */
  apr_pool_cleanup_register(lock->pool, lock, pool_cleanup,
                            pool_cleanup_child);
  *adm_access = lock;

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_adm_open3(svn_wc_adm_access_t **adm_access,
                 svn_wc_adm_access_t *associated,
                 const char *path,
                 svn_boolean_t write_lock,
                 int levels_to_lock,
                 svn_cancel_func_t cancel_func,
                 void *cancel_baton,
                 apr_pool_t *pool)
{
  /* Make sure that ASSOCIATED has a set of access batons, so that we can
     glom a reference to self into it. */
  if (associated)
    adm_ensure_set(associated);

  return do_open(adm_access, associated ? associated->shared : NULL, path,
                 write_lock, levels_to_lock, FALSE,
                 cancel_func, cancel_baton, pool);
}

svn_error_t *
svn_wc__adm_pre_open(svn_wc_adm_access_t **adm_access,
                     const char *path,
                     apr_pool_t *pool)
{
  return do_open(adm_access, NULL, path, TRUE, 0, TRUE, NULL, NULL, pool);
}

svn_error_t *
svn_wc_adm_probe_open3(svn_wc_adm_access_t **adm_access,
                       svn_wc_adm_access_t *associated,
                       const char *path,
                       svn_boolean_t write_lock,
                       int levels_to_lock,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *pool)
{
  svn_error_t *err;
  const char *dir;
  int wc_format;

  SVN_ERR(probe(&dir, path, &wc_format, pool));

  /* If we moved up a directory, then the path is not a directory, or it
     is not under version control. In either case, the notion of
     levels_to_lock does not apply to the provided path.  Disable it so
     that we don't end up trying to lock more than we need.  */
  if (dir != path)
    levels_to_lock = 0;

  err = svn_wc_adm_open3(adm_access, associated, dir, write_lock,
                         levels_to_lock, cancel_func, cancel_baton, pool);
  if (err)
    {
      svn_error_t *err2;

      /* If we got an error on the parent dir, that means we failed to
         get an access baton for the child in the first place.  And if
         the reason we couldn't get the child access baton is that the
         child is not a versioned directory, then return an error
         about the child, not the parent. */
      svn_node_kind_t child_kind;
      if ((err2 = svn_io_check_path(path, &child_kind, pool)))
        {
          svn_error_compose(err, err2);
          return err;
        }

      if ((dir != path)
          && (child_kind == svn_node_dir)
          && (err->apr_err == SVN_ERR_WC_NOT_DIRECTORY))
        {
          svn_error_clear(err);
          return svn_error_createf(SVN_ERR_WC_NOT_DIRECTORY, NULL,
                                   _("'%s' is not a working copy"),
                                   svn_path_local_style(path, pool));
        }
      else
        {
          return err;
        }
    }

  if (wc_format && ! (*adm_access)->wc_format)
    (*adm_access)->wc_format = wc_format;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__adm_retrieve_internal(svn_wc_adm_access_t **adm_access,
                              svn_wc_adm_access_t *associated,
                              const char *path,
                              apr_pool_t *pool)
{
  if (associated->shared && associated->shared->set)
    *adm_access = apr_hash_get(associated->shared->set,
                               path, APR_HASH_KEY_STRING);
  else if (! strcmp(associated->path, path))
    *adm_access = associated;
  else
    *adm_access = NULL;

  if (*adm_access == &missing)
    *adm_access = NULL;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_adm_retrieve(svn_wc_adm_access_t **adm_access,
                    svn_wc_adm_access_t *associated,
                    const char *path,
                    apr_pool_t *pool)
{
  SVN_ERR(svn_wc__adm_retrieve_internal(adm_access, associated, path, pool));

  /* Most of the code expects access batons to exist, so returning an error
     generally makes the calling code simpler as it doesn't need to check
     for NULL batons. */
  if (! *adm_access)
    {
      const char *wcpath;
      const svn_wc_entry_t *subdir_entry;
      svn_node_kind_t wckind;
      svn_node_kind_t kind;
      svn_error_t *err;

      err = svn_wc_entry(&subdir_entry, path, associated, TRUE, pool);

      /* If we can't get an entry here, we are in pretty bad shape,
         and will have to fall back to using just regular old paths to
         see what's going on.  */
      if (err)
        {
          svn_error_clear(err);
          subdir_entry = NULL;
        }

      err = svn_io_check_path(path, &kind, pool);

      /* If we can't check the path, we can't make a good error
         message.  */
      if (err)
        {
          return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, err,
                                   _("Unable to check path existence for '%s'"),
                                   svn_path_local_style(path, pool));
        }

      if (subdir_entry)
        {
          if (subdir_entry->kind == svn_node_dir
              && kind == svn_node_file)
            {
              const char *err_msg = apr_psprintf
                (pool, _("Expected '%s' to be a directory but found a file"),
                 svn_path_local_style(path, pool));
              return svn_error_create(SVN_ERR_WC_NOT_LOCKED,
                                      svn_error_create
                                        (SVN_ERR_WC_NOT_DIRECTORY, NULL,
                                         err_msg),
                                      err_msg);
            }
          else if (subdir_entry->kind == svn_node_file
                   && kind == svn_node_dir)
            {
              const char *err_msg = apr_psprintf
                (pool, _("Expected '%s' to be a file but found a directory"),
                 svn_path_local_style(path, pool));
              return svn_error_create(SVN_ERR_WC_NOT_LOCKED,
                                      svn_error_create(SVN_ERR_WC_NOT_FILE,
                                                       NULL, err_msg),
                                      err_msg);
            }
        }

      wcpath = svn_wc__adm_child(path, NULL, pool);
      err = svn_io_check_path(wcpath, &wckind, pool);

      /* If we can't check the path, we can't make a good error
         message.  */
      if (err)
        {
          return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, err,
                                   _("Unable to check path existence for '%s'"),
                                   svn_path_local_style(wcpath, pool));
        }

      if (kind == svn_node_none)
        {
          const char *err_msg = apr_psprintf(pool,
                                             _("Directory '%s' is missing"),
                                             svn_path_local_style(path, pool));
          return svn_error_create(SVN_ERR_WC_NOT_LOCKED,
                                  svn_error_create(SVN_ERR_WC_PATH_NOT_FOUND,
                                                   NULL, err_msg),
                                  err_msg);
        }

      else if (kind == svn_node_dir && wckind == svn_node_none)
        return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, NULL,
                                 _("Directory '%s' containing working copy admin area is missing"),
                                 svn_path_local_style(wcpath, pool));

      else if (kind == svn_node_dir && wckind == svn_node_dir)
        return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, NULL,
                                 _("Unable to lock '%s'"),
                                 svn_path_local_style(path, pool));

      /* If all else fails, return our useless generic error.  */
      return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, NULL,
                               _("Working copy '%s' is not locked"),
                               svn_path_local_style(path, pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_adm_probe_retrieve(svn_wc_adm_access_t **adm_access,
                          svn_wc_adm_access_t *associated,
                          const char *path,
                          apr_pool_t *pool)
{
  const char *dir;
  const svn_wc_entry_t *entry;
  int wc_format;
  svn_error_t *err;

  SVN_ERR(svn_wc_entry(&entry, path, associated, TRUE, pool));

  if (! entry)
    /* Not a versioned item, probe it */
    SVN_ERR(probe(&dir, path, &wc_format, pool));
  else if (entry->kind != svn_node_dir)
    dir = svn_path_dirname(path, pool);
  else
    dir = path;

  err = svn_wc_adm_retrieve(adm_access, associated, dir, pool);
  if (err && err->apr_err == SVN_ERR_WC_NOT_LOCKED)
    {
      /* We'll receive a NOT LOCKED error for various reasons,
         including the reason we'll actually want to test for:
         The path is a versioned directory, but missing, in which case
         we want its parent's adm_access (which holds minimal data
         on the child) */
      svn_error_clear(err);
      SVN_ERR(probe(&dir, path, &wc_format, pool));
      SVN_ERR(svn_wc_adm_retrieve(adm_access, associated, dir, pool));
    }
  else
    return err;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_adm_probe_try2(svn_wc_adm_access_t **adm_access,
                      svn_wc_adm_access_t *associated,
                      const char *path,
                      svn_boolean_t write_lock,
                      int levels_to_lock,
                      apr_pool_t *pool)
{
  return svn_wc_adm_probe_try3(adm_access, associated, path, write_lock,
                               levels_to_lock, NULL, NULL, pool);
}

svn_error_t *
svn_wc_adm_probe_try3(svn_wc_adm_access_t **adm_access,
                      svn_wc_adm_access_t *associated,
                      const char *path,
                      svn_boolean_t write_lock,
                      int levels_to_lock,
                      svn_cancel_func_t cancel_func,
                      void *cancel_baton,
                      apr_pool_t *pool)
{
  svn_error_t *err;

  err = svn_wc_adm_probe_retrieve(adm_access, associated, path, pool);

  /* SVN_ERR_WC_NOT_LOCKED would mean there was no access baton for
     path in associated, in which case we want to open an access
     baton and add it to associated. */
  if (err && (err->apr_err == SVN_ERR_WC_NOT_LOCKED))
    {
      svn_error_clear(err);
      err = svn_wc_adm_probe_open3(adm_access, associated,
                                   path, write_lock, levels_to_lock,
                                   cancel_func, cancel_baton,
                                   svn_wc_adm_access_pool(associated));

      /* If the path is not a versioned directory, we just return a
         null access baton with no error.  Note that of the errors we
         do report, the most important (and probably most likely) is
         SVN_ERR_WC_LOCKED.  That error would mean that someone else
         has this area locked, and we definitely want to bail in that
         case. */
      if (err && (err->apr_err == SVN_ERR_WC_NOT_DIRECTORY))
        {
          svn_error_clear(err);
          *adm_access = NULL;
          err = NULL;
        }
    }

  return err;
}

/* A helper for svn_wc_adm_open_anchor.  Add all the access batons in the
   T_ACCESS set, including T_ACCESS, to the P_ACCESS set. */
static void join_batons(svn_wc_adm_access_t *p_access,
                        svn_wc_adm_access_t *t_access,
                        apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  adm_ensure_set(p_access);
  if (t_access->shared == NULL || t_access->shared->set == NULL)
    {
      t_access->shared = p_access->shared;
      apr_hash_set(p_access->shared->set, t_access->path, APR_HASH_KEY_STRING,
                   t_access);
      return;
    }

  for (hi = apr_hash_first(pool, t_access->shared->set);
       hi;
       hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_wc_adm_access_t *adm_access;

      apr_hash_this(hi, &key, NULL, &val);
      adm_access = val;
      if (adm_access != &missing)
        adm_access->shared = p_access->shared;
      apr_hash_set(p_access->shared->set, key, APR_HASH_KEY_STRING, adm_access);
    }
  t_access->set_owner = FALSE;
}

svn_error_t *
svn_wc_adm_open_anchor(svn_wc_adm_access_t **anchor_access,
                       svn_wc_adm_access_t **target_access,
                       const char **target,
                       const char *path,
                       svn_boolean_t write_lock,
                       int levels_to_lock,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *pool)
{
  const char *base_name = svn_path_basename(path, pool);

  if (svn_path_is_empty(path)
      || svn_dirent_is_root(path, strlen(path))
      || ! strcmp(base_name, ".."))
    {
      SVN_ERR(do_open(anchor_access, NULL, path, write_lock, levels_to_lock,
                      FALSE, cancel_func, cancel_baton, pool));
      *target_access = *anchor_access;
      *target = "";
    }
  else
    {
      svn_error_t *err;
      svn_wc_adm_access_t *p_access, *t_access;
      const char *parent = svn_path_dirname(path, pool);
      svn_error_t *p_access_err = SVN_NO_ERROR;

      /* Try to open parent of PATH to setup P_ACCESS */
      err = do_open(&p_access, NULL, parent, write_lock, 0, FALSE,
                    cancel_func, cancel_baton, pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_WC_NOT_DIRECTORY)
            {
              svn_error_clear(err);
              p_access = NULL;
            }
          else if (write_lock && (err->apr_err == SVN_ERR_WC_LOCKED
                                  || APR_STATUS_IS_EACCES(err->apr_err)))
            {
              /* If P_ACCESS isn't to be returned then a read-only baton
                 will do for now, but keep the error in case we need it. */
              svn_error_t *err2 = do_open(&p_access, NULL, parent, FALSE, 0,
                                          FALSE, cancel_func, cancel_baton,
                                          pool);
              if (err2)
                {
                  svn_error_clear(err2);
                  return err;
                }
              p_access_err = err;
            }
          else
            return err;
        }

      /* Try to open PATH to setup T_ACCESS */
      err = do_open(&t_access, NULL, path, write_lock, levels_to_lock,
                    FALSE, cancel_func, cancel_baton, pool);
      if (err)
        {
          if (! p_access || err->apr_err != SVN_ERR_WC_NOT_DIRECTORY)
            {
              if (p_access)
                svn_error_clear(do_close(p_access, FALSE, TRUE, pool));
              svn_error_clear(p_access_err);
              return err;
            }

          svn_error_clear(err);
          t_access = NULL;
        }

      /* At this stage might have P_ACCESS, T_ACCESS or both */

      /* Check for switched or disjoint P_ACCESS and T_ACCESS */
      if (p_access && t_access)
        {
          const svn_wc_entry_t *t_entry, *p_entry, *t_entry_in_p;

          err = svn_wc_entry(&t_entry_in_p, path, p_access, FALSE, pool);
          if (! err)
            err = svn_wc_entry(&t_entry, path, t_access, FALSE, pool);
          if (! err)
            err = svn_wc_entry(&p_entry, parent, p_access, FALSE, pool);
          if (err)
            {
              svn_error_clear(p_access_err);
              svn_error_clear(do_close(p_access, FALSE, TRUE, pool));
              svn_error_clear(do_close(t_access, FALSE, TRUE, pool));
              return err;
            }

          /* Disjoint won't have PATH in P_ACCESS, switched will have
             incompatible URLs */
          if (! t_entry_in_p
              ||
              (p_entry->url && t_entry->url
               && (strcmp(svn_path_dirname(t_entry->url, pool), p_entry->url)
                   || strcmp(svn_path_uri_encode(base_name, pool),
                             svn_path_basename(t_entry->url, pool)))))
            {
              /* Switched or disjoint, so drop P_ACCESS */
              err = do_close(p_access, FALSE, TRUE, pool);
              if (err)
                {
                  svn_error_clear(p_access_err);
                  svn_error_clear(do_close(t_access, FALSE, TRUE, pool));
                  return err;
                }
              p_access = NULL;
            }
        }

      if (p_access)
        {
          if (p_access_err)
            {
              /* Need P_ACCESS, so the read-only temporary won't do */
              if (t_access)
                svn_error_clear(do_close(t_access, FALSE, TRUE, pool));
              svn_error_clear(do_close(p_access, FALSE, TRUE, pool));
              return p_access_err;
            }
          else if (t_access)
            join_batons(p_access, t_access, pool);
        }
      svn_error_clear(p_access_err);

      if (! t_access)
        {
          const svn_wc_entry_t *t_entry;
          err = svn_wc_entry(&t_entry, path, p_access, FALSE, pool);
          if (err)
            {
              if (p_access)
                svn_error_clear(do_close(p_access, FALSE, TRUE, pool));
              return err;
            }
          if (t_entry && t_entry->kind == svn_node_dir)
            {
              adm_ensure_set(p_access);
              apr_hash_set(p_access->shared->set,
                           apr_pstrdup(p_access->pool, path),
                           APR_HASH_KEY_STRING, &missing);
            }
        }

      *anchor_access = p_access ? p_access : t_access;
      *target_access = t_access ? t_access : p_access;

      if (! p_access)
        *target = "";
      else
        *target = base_name;
    }

  return SVN_NO_ERROR;
}


/* Does the work of closing the access baton ADM_ACCESS.  Any physical
   locks are removed from the working copy if PRESERVE_LOCK is FALSE, or
   are left if PRESERVE_LOCK is TRUE.  Any associated access batons that
   are direct descendants will also be closed.

   ### FIXME: If the set has a "hole", say it contains locks for the
   ### directories A, A/B, A/B/C/X but not A/B/C then closing A/B will not
   ### reach A/B/C/X .
 */
static svn_error_t *
do_close(svn_wc_adm_access_t *adm_access,
         svn_boolean_t preserve_lock,
         svn_boolean_t recurse,
         apr_pool_t *scratch_pool)
{
  if (adm_access->type == svn_wc__adm_access_closed)
    return SVN_NO_ERROR;

  /* Close descendant batons */
  if (recurse && adm_access->shared && adm_access->shared->set)
    {
      int i;
      apr_array_header_t *children
        = svn_sort__hash(adm_access->shared->set,
                         svn_sort_compare_items_as_paths,
                         scratch_pool);

      /* Go backwards through the list to close children before their
         parents. */
      for (i = children->nelts - 1; i >= 0; --i)
        {
          svn_sort__item_t *item = &APR_ARRAY_IDX(children, i,
                                                  svn_sort__item_t);
          const char *path = item->key;
          svn_wc_adm_access_t *child = item->value;

          if (child == &missing)
            {
              /* We don't close the missing entry, but get rid of it from
                 the set. */
              apr_hash_set(adm_access->shared->set,
                           path, APR_HASH_KEY_STRING, NULL);
              continue;
            }

          if (! svn_path_is_ancestor(adm_access->path, path)
              || strcmp(adm_access->path, path) == 0)
            continue;

          SVN_ERR(do_close(child, preserve_lock, FALSE, scratch_pool));
        }
    }

  /* Physically unlock if required */
  if (adm_access->type == svn_wc__adm_access_write_lock)
    {
      if (adm_access->lock_exists && ! preserve_lock)
        {
          /* Remove the physical lock in the admin directory for
             PATH. It is acceptable for the administrative area to
             have disappeared, such as when the directory is removed
             from the working copy.  It is an error for the lock to
             have disappeared if the administrative area still exists. */

          svn_error_t *err = svn_wc__remove_adm_file(adm_access,
                                                     SVN_WC__ADM_LOCK,
                                                     scratch_pool);
          if (err)
            {
              if (svn_wc__adm_area_exists(adm_access, scratch_pool))
                return err;
              svn_error_clear(err);
            }

          adm_access->lock_exists = FALSE;
        }
    }

  /* Reset to prevent further use of the lock. */
  adm_access->type = svn_wc__adm_access_closed;

  /* Detach from set */
  if (adm_access->shared && adm_access->shared->set)
    {
      apr_hash_set(adm_access->shared->set,
                   adm_access->path, APR_HASH_KEY_STRING,
                   NULL);

      SVN_ERR_ASSERT(! adm_access->set_owner
                     || apr_hash_count(adm_access->shared->set) == 0);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_adm_close2(svn_wc_adm_access_t *adm_access, apr_pool_t *scratch_pool)
{
  /* ### a scratch pool should be passed */
  return do_close(adm_access, FALSE, TRUE, scratch_pool);
}

svn_boolean_t
svn_wc_adm_locked(const svn_wc_adm_access_t *adm_access)
{
  return adm_access->type == svn_wc__adm_access_write_lock;
}

svn_error_t *
svn_wc__adm_write_check(const svn_wc_adm_access_t *adm_access,
                        apr_pool_t *scratch_pool)
{
  if (adm_access->type == svn_wc__adm_access_write_lock)
    {
      if (adm_access->lock_exists)
        {
          /* Check physical lock still exists and hasn't been stolen.  This
             really is paranoia, I have only ever seen one report of this
             triggering (from someone using the 0.25 release) and that was
             never reproduced.  The check accesses the physical filesystem
             so it is expensive, but it only runs when we are going to
             modify the admin area.  If it ever proves to be a bottleneck
             the physical check could be removed, just leaving the logical
             check. */
          svn_boolean_t locked;

          SVN_ERR(svn_wc_locked(&locked, adm_access->path, scratch_pool));
          if (! locked)
            return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, NULL,
                                     _("Write-lock stolen in '%s'"),
                                     svn_path_local_style(adm_access->path,
                                                          scratch_pool));
        }
    }
  else
    {
      return svn_error_createf(SVN_ERR_WC_NOT_LOCKED, NULL,
                               _("No write-lock in '%s'"),
                               svn_path_local_style(adm_access->path,
                                                    scratch_pool));
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_locked(svn_boolean_t *locked, const char *path, apr_pool_t *pool)
{
  svn_node_kind_t kind;
  const char *lockfile = svn_wc__adm_child(path, SVN_WC__ADM_LOCK, pool);

  SVN_ERR(svn_io_check_path(lockfile, &kind, pool));
  if (kind == svn_node_file)
    *locked = TRUE;
  else if (kind == svn_node_none)
    *locked = FALSE;
  else
    return svn_error_createf(SVN_ERR_WC_LOCKED, NULL,
                             _("Lock file '%s' is not a regular file"),
                             svn_path_local_style(lockfile, pool));

  return SVN_NO_ERROR;
}


const char *
svn_wc_adm_access_path(const svn_wc_adm_access_t *adm_access)
{
  return adm_access->path;
}


apr_pool_t *
svn_wc_adm_access_pool(const svn_wc_adm_access_t *adm_access)
{
  return adm_access->pool;
}


svn_error_t *
svn_wc__adm_is_cleanup_required(svn_boolean_t *cleanup,
                                const svn_wc_adm_access_t *adm_access,
                                apr_pool_t *pool)
{
  if (adm_access->type == svn_wc__adm_access_write_lock)
    {
      svn_node_kind_t kind;
      const char *log_path = svn_wc__adm_child(adm_access->path,
                                               SVN_WC__ADM_LOG, pool);

      /* The presence of a log file demands cleanup */
      SVN_ERR(svn_io_check_path(log_path, &kind, pool));
      *cleanup = (kind == svn_node_file);
    }
  else
    *cleanup = FALSE;

  return SVN_NO_ERROR;
}

/* Ensure that the cache for the pruned hash (no deleted entries) in
   ADM_ACCESS is valid if the full hash is cached.  POOL is used for
   local, short term, memory allocation.

   ### Should this sort of processing be in entries.c? */
static void
prune_deleted(svn_wc_adm_access_t *adm_access,
              apr_pool_t *pool)
{
  if (! adm_access->entries && adm_access->entries_hidden)
    {
      apr_hash_index_t *hi;

      /* I think it will be common for there to be no deleted entries, so
         it is worth checking for that case as we can optimise it. */
      for (hi = apr_hash_first(pool, adm_access->entries_hidden);
           hi;
           hi = apr_hash_next(hi))
        {
          void *val;
          const svn_wc_entry_t *entry;
          apr_hash_this(hi, NULL, NULL, &val);
          entry = val;
          if ((entry->deleted
               && (entry->schedule != svn_wc_schedule_add)
               && (entry->schedule != svn_wc_schedule_replace))
              || entry->absent || (entry->depth == svn_depth_exclude))
            break;
        }

      if (! hi)
        {
          /* There are no deleted entries, so we can use the full hash */
          adm_access->entries = adm_access->entries_hidden;
          return;
        }

      /* Construct pruned hash without deleted entries */
      adm_access->entries = apr_hash_make(adm_access->pool);
      for (hi = apr_hash_first(pool, adm_access->entries_hidden);
           hi;
           hi = apr_hash_next(hi))
        {
          void *val;
          const void *key;
          const svn_wc_entry_t *entry;

          apr_hash_this(hi, &key, NULL, &val);
          entry = val;
          if (((entry->deleted == FALSE) && (entry->absent == FALSE)
               && (entry->depth != svn_depth_exclude))
              || (entry->schedule == svn_wc_schedule_add)
              || (entry->schedule == svn_wc_schedule_replace))
            {
              apr_hash_set(adm_access->entries, key,
                           APR_HASH_KEY_STRING, entry);
            }
        }
    }
}


void
svn_wc__adm_access_set_entries(svn_wc_adm_access_t *adm_access,
                               svn_boolean_t show_hidden,
                               apr_hash_t *entries)
{
  if (show_hidden)
    adm_access->entries_hidden = entries;
  else
    adm_access->entries = entries;
}


apr_hash_t *
svn_wc__adm_access_entries(svn_wc_adm_access_t *adm_access,
                           svn_boolean_t show_hidden,
                           apr_pool_t *pool)
{
  if (! show_hidden)
    {
      prune_deleted(adm_access, pool);
      return adm_access->entries;
    }
  else
    return adm_access->entries_hidden;
}

void
svn_wc__adm_access_set_wcprops(svn_wc_adm_access_t *adm_access,
                        apr_hash_t *wcprops)
{
  adm_access->wcprops = wcprops;
}

apr_hash_t *
svn_wc__adm_access_wcprops(const svn_wc_adm_access_t *adm_access)
{
  return adm_access->wcprops;
}


int
svn_wc__adm_wc_format(const svn_wc_adm_access_t *adm_access)
{
  return adm_access->wc_format;
}

void
svn_wc__adm_set_wc_format(svn_wc_adm_access_t *adm_access,
                          int format)
{
  adm_access->wc_format = format;
}


svn_boolean_t
svn_wc__adm_missing(const svn_wc_adm_access_t *adm_access,
                    const char *path)
{
  if (adm_access->shared
      && adm_access->shared->set
      && apr_hash_get(adm_access->shared->set,
                      path, APR_HASH_KEY_STRING) == &missing)
    return TRUE;

  return FALSE;
}


/* Extend the scope of the svn_wc_adm_access_t * passed in as WALK_BATON
   for its entire WC tree.  An implementation of
   svn_wc_entry_callbacks2_t's found_entry() API. */
static svn_error_t *
extend_lock_found_entry(const char *path,
                        const svn_wc_entry_t *entry,
                        void *walk_baton,
                        apr_pool_t *pool)
{
  /* If PATH is a directory, and it's not already locked, lock it all
     the way down to its leaf nodes. */
  if (entry->kind == svn_node_dir &&
      strcmp(entry->name, SVN_WC_ENTRY_THIS_DIR) != 0)
    {
      svn_wc_adm_access_t *anchor_access = walk_baton, *adm_access;
      svn_boolean_t write_lock =
        (anchor_access->type == svn_wc__adm_access_write_lock);
      svn_error_t *err = svn_wc_adm_probe_try3(&adm_access, anchor_access,
                                               path, write_lock, -1,
                                               NULL, NULL, pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_WC_LOCKED)
            /* Good!  The directory is *already* locked... */
            svn_error_clear(err);
          else
            return err;
        }
    }
  return SVN_NO_ERROR;
}


/* WC entry walker callbacks for svn_wc__adm_extend_lock_to_tree(). */
static const svn_wc_entry_callbacks2_t extend_lock_walker =
  {
    extend_lock_found_entry,
    svn_wc__walker_default_error_handler
  };


svn_error_t *
svn_wc__adm_extend_lock_to_tree(svn_wc_adm_access_t *adm_access,
                                apr_pool_t *pool)
{
  return svn_wc_walk_entries3(adm_access->path, adm_access,
                              &extend_lock_walker, adm_access,
                              svn_depth_infinity, FALSE, NULL, NULL, pool);
}

