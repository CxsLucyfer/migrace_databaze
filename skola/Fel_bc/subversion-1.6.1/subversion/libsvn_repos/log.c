/* log.c --- retrieving log messages
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


#include <stdlib.h>
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_compat.h"
#include "svn_private_config.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_path.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_sorts.h"
#include "svn_props.h"
#include "svn_mergeinfo.h"
#include "repos.h"



svn_error_t *
svn_repos_check_revision_access(svn_repos_revision_access_level_t *access_level,
                                svn_repos_t *repos,
                                svn_revnum_t revision,
                                svn_repos_authz_func_t authz_read_func,
                                void *authz_read_baton,
                                apr_pool_t *pool)
{
  svn_fs_t *fs = svn_repos_fs(repos);
  svn_fs_root_t *rev_root;
  apr_hash_t *changes;
  apr_hash_index_t *hi;
  svn_boolean_t found_readable = FALSE;
  svn_boolean_t found_unreadable = FALSE;
  apr_pool_t *subpool;

  /* By default, we'll grant full read access to REVISION. */
  *access_level = svn_repos_revision_access_full;

  /* No auth-checking function?  We're done. */
  if (! authz_read_func)
    return SVN_NO_ERROR;

  /* Fetch the changes associated with REVISION. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, revision, pool));
  SVN_ERR(svn_fs_paths_changed2(&changes, rev_root, pool));

  /* No changed paths?  We're done. */
  if (apr_hash_count(changes) == 0)
    return SVN_NO_ERROR;

  /* Otherwise, we have to check the readability of each changed
     path, or at least enough to answer the question asked. */
  subpool = svn_pool_create(pool);
  for (hi = apr_hash_first(NULL, changes); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_fs_path_change2_t *change;
      svn_boolean_t readable;

      svn_pool_clear(subpool);
      apr_hash_this(hi, &key, NULL, &val);
      change = val;

      SVN_ERR(authz_read_func(&readable, rev_root, key,
                              authz_read_baton, subpool));
      if (! readable)
        found_unreadable = TRUE;
      else
        found_readable = TRUE;

      /* If we have at least one of each (readable/unreadable), we
         have our answer. */
      if (found_readable && found_unreadable)
        goto decision;

      switch (change->change_kind)
        {
        case svn_fs_path_change_add:
        case svn_fs_path_change_replace:
          {
            const char *copyfrom_path;
            svn_revnum_t copyfrom_rev;

            SVN_ERR(svn_fs_copied_from(&copyfrom_rev, &copyfrom_path,
                                       rev_root, key, subpool));
            if (copyfrom_path && SVN_IS_VALID_REVNUM(copyfrom_rev))
              {
                svn_fs_root_t *copyfrom_root;
                SVN_ERR(svn_fs_revision_root(&copyfrom_root, fs,
                                             copyfrom_rev, subpool));
                SVN_ERR(authz_read_func(&readable,
                                        copyfrom_root, copyfrom_path,
                                        authz_read_baton, subpool));
                if (! readable)
                  found_unreadable = TRUE;

                /* If we have at least one of each (readable/unreadable), we
                   have our answer. */
                if (found_readable && found_unreadable)
                  goto decision;
              }
          }
          break;

        case svn_fs_path_change_delete:
        case svn_fs_path_change_modify:
        default:
          break;
        }
    }

 decision:
  svn_pool_destroy(subpool);

  /* Either every changed path was unreadable... */
  if (! found_readable)
    *access_level = svn_repos_revision_access_none;

  /* ... or some changed path was unreadable... */
  else if (found_unreadable)
    *access_level = svn_repos_revision_access_partial;

  /* ... or every changed path was readable (the default). */
  return SVN_NO_ERROR;
}


/* Store as keys in CHANGED the paths of all node in ROOT that show a
 * significant change.  "Significant" means that the text or
 * properties of the node were changed, or that the node was added or
 * deleted.
 *
 * The CHANGED hash set and its keys and values are allocated in POOL;
 * keys are const char * paths and values are svn_log_changed_path_t.
 *
 * If optional AUTHZ_READ_FUNC is non-NULL, then use it (with
 * AUTHZ_READ_BATON and FS) to check whether each changed-path (and
 * copyfrom_path) is readable:
 *
 *     - If some paths are readable and some are not, then silently
 *     omit the unreadable paths from the CHANGED hash, and return
 *     SVN_ERR_AUTHZ_PARTIALLY_READABLE.
 *
 *     - If absolutely every changed-path (and copyfrom_path) is
 *     unreadable, then return an empty CHANGED hash and
 *     SVN_ERR_AUTHZ_UNREADABLE.  (This is to distinguish a revision
 *     which truly has no changed paths from a revision in which all
 *     paths are unreadable.)
 */
static svn_error_t *
detect_changed(apr_hash_t **changed,
               svn_fs_root_t *root,
               svn_fs_t *fs,
               svn_repos_authz_func_t authz_read_func,
               void *authz_read_baton,
               apr_pool_t *pool)
{
  apr_hash_t *changes;
  apr_hash_index_t *hi;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_boolean_t found_readable = FALSE;
  svn_boolean_t found_unreadable = FALSE;

  *changed = apr_hash_make(pool);
  SVN_ERR(svn_fs_paths_changed2(&changes, root, pool));

  if (apr_hash_count(changes) == 0)
    /* No paths changed in this revision?  Uh, sure, I guess the
       revision is readable, then.  */
    return SVN_NO_ERROR;

  for (hi = apr_hash_first(pool, changes); hi; hi = apr_hash_next(hi))
    {
      /* NOTE:  Much of this loop is going to look quite similar to
         svn_repos_check_revision_access(), but we have to do more things
         here, so we'll live with the duplication. */
      const void *key;
      void *val;
      svn_fs_path_change2_t *change;
      const char *path;
      char action;
      svn_log_changed_path2_t *item;

      svn_pool_clear(subpool);

      /* KEY will be the path, VAL the change. */
      apr_hash_this(hi, &key, NULL, &val);
      path = (const char *) key;
      change = val;

      /* Skip path if unreadable. */
      if (authz_read_func)
        {
          svn_boolean_t readable;
          SVN_ERR(authz_read_func(&readable,
                                  root, path,
                                  authz_read_baton, subpool));
          if (! readable)
            {
              found_unreadable = TRUE;
              continue;
            }
        }

      /* At least one changed-path was readable. */
      found_readable = TRUE;

      switch (change->change_kind)
        {
        case svn_fs_path_change_reset:
          continue;

        case svn_fs_path_change_add:
          action = 'A';
          break;

        case svn_fs_path_change_replace:
          action = 'R';
          break;

        case svn_fs_path_change_delete:
          action = 'D';
          break;

        case svn_fs_path_change_modify:
        default:
          action = 'M';
          break;
        }

      item = svn_log_changed_path2_create(pool);
      item->action = action;
      item->node_kind = change->node_kind;
      item->copyfrom_rev = SVN_INVALID_REVNUM;
      if ((action == 'A') || (action == 'R'))
        {
          const char *copyfrom_path;
          svn_revnum_t copyfrom_rev;

          SVN_ERR(svn_fs_copied_from(&copyfrom_rev, &copyfrom_path,
                                     root, path, subpool));

          if (copyfrom_path && SVN_IS_VALID_REVNUM(copyfrom_rev))
            {
              svn_boolean_t readable = TRUE;

              if (authz_read_func)
                {
                  svn_fs_root_t *copyfrom_root;

                  SVN_ERR(svn_fs_revision_root(&copyfrom_root, fs,
                                               copyfrom_rev, subpool));
                  SVN_ERR(authz_read_func(&readable,
                                          copyfrom_root, copyfrom_path,
                                          authz_read_baton, subpool));
                  if (! readable)
                    found_unreadable = TRUE;
                }

              if (readable)
                {
                  item->copyfrom_path = apr_pstrdup(pool, copyfrom_path);
                  item->copyfrom_rev = copyfrom_rev;
                }
            }
        }
      apr_hash_set(*changed, apr_pstrdup(pool, path),
                   APR_HASH_KEY_STRING, item);
    }

  svn_pool_destroy(subpool);

  if (! found_readable)
    /* Every changed-path was unreadable. */
    return svn_error_create(SVN_ERR_AUTHZ_UNREADABLE,
                            NULL, NULL);

  if (found_unreadable)
    /* At least one changed-path was unreadable. */
    return svn_error_create(SVN_ERR_AUTHZ_PARTIALLY_READABLE,
                            NULL, NULL);

  /* Every changed-path was readable. */
  return SVN_NO_ERROR;
}

/* This is used by svn_repos_get_logs to keep track of multiple
 * path history information while working through history.
 *
 * The two pools are swapped after each iteration through history because
 * to get the next history requires the previous one.
 */
struct path_info
{
  svn_stringbuf_t *path;
  svn_revnum_t history_rev;
  svn_boolean_t done;
  svn_boolean_t first_time;

  /* If possible, we like to keep open the history object for each path,
     since it avoids needed to open and close it many times as we walk
     backwards in time.  To do so we need two pools, so that we can clear
     one each time through.  If we're not holding the history open for
     this path then these three pointers will be NULL. */
  svn_fs_history_t *hist;
  apr_pool_t *newpool;
  apr_pool_t *oldpool;
};

/* Advance to the next history for the path.
 *
 * If INFO->HIST is not NULL we do this using that existing history object,
 * otherwise we open a new one.
 *
 * If no more history is available or the history revision is less
 * (earlier) than START, or the history is not available due
 * to authorization, then INFO->DONE is set to TRUE.
 *
 * A STRICT value of FALSE will indicate to follow history across copied
 * paths.
 *
 * If optional AUTHZ_READ_FUNC is non-NULL, then use it (with
 * AUTHZ_READ_BATON and FS) to check whether INFO->PATH is still readable if
 * we do indeed find more history for the path.
 */
static svn_error_t *
get_history(struct path_info *info,
            svn_fs_t *fs,
            svn_boolean_t strict,
            svn_repos_authz_func_t authz_read_func,
            void *authz_read_baton,
            svn_revnum_t start,
            apr_pool_t *pool)
{
  svn_fs_root_t *history_root = NULL;
  svn_fs_history_t *hist;
  apr_pool_t *subpool;
  const char *path;

  if (info->hist)
    {
      subpool = info->newpool;

      SVN_ERR(svn_fs_history_prev(&info->hist, info->hist, ! strict, subpool));

      hist = info->hist;
    }
  else
    {
      subpool = svn_pool_create(pool);

      /* Open the history located at the last rev we were at. */
      SVN_ERR(svn_fs_revision_root(&history_root, fs, info->history_rev,
                                   subpool));

      SVN_ERR(svn_fs_node_history(&hist, history_root, info->path->data,
                                  subpool));

      SVN_ERR(svn_fs_history_prev(&hist, hist, ! strict, subpool));

      if (info->first_time)
        info->first_time = FALSE;
      else
        SVN_ERR(svn_fs_history_prev(&hist, hist, ! strict, subpool));
    }

  if (! hist)
    {
      svn_pool_destroy(subpool);
      if (info->oldpool)
        svn_pool_destroy(info->oldpool);
      info->done = TRUE;
      return SVN_NO_ERROR;
    }

  /* Fetch the location information for this history step. */
  SVN_ERR(svn_fs_history_location(&path, &info->history_rev,
                                  hist, subpool));

  svn_stringbuf_set(info->path, path);

  /* If this history item predates our START revision then
     don't fetch any more for this path. */
  if (info->history_rev < start)
    {
      svn_pool_destroy(subpool);
      if (info->oldpool)
        svn_pool_destroy(info->oldpool);
      info->done = TRUE;
      return SVN_NO_ERROR;
    }

  /* Is the history item readable?  If not, done with path. */
  if (authz_read_func)
    {
      svn_boolean_t readable;
      SVN_ERR(svn_fs_revision_root(&history_root, fs,
                                   info->history_rev,
                                   subpool));
      SVN_ERR(authz_read_func(&readable, history_root,
                              info->path->data,
                              authz_read_baton,
                              subpool));
      if (! readable)
        info->done = TRUE;
    }

  if (! info->hist)
    {
      svn_pool_destroy(subpool);
    }
  else
    {
      apr_pool_t *temppool = info->oldpool;
      info->oldpool = info->newpool;
      svn_pool_clear(temppool);
      info->newpool = temppool;
    }

  return SVN_NO_ERROR;
}

/* Set INFO->HIST to the next history for the path *if* there is history
 * available and INFO->HISTORY_REV is equal to or greater than CURRENT.
 *
 * *CHANGED is set to TRUE if the path has history in the CURRENT revision,
 * otherwise it is not touched.
 *
 * If we do need to get the next history revision for the path, call
 * get_history to do it -- see it for details.
 */
static svn_error_t *
check_history(svn_boolean_t *changed,
              struct path_info *info,
              svn_fs_t *fs,
              svn_revnum_t current,
              svn_boolean_t strict,
              svn_repos_authz_func_t authz_read_func,
              void *authz_read_baton,
              svn_revnum_t start,
              apr_pool_t *pool)
{
  /* If we're already done with histories for this path,
     don't try to fetch any more. */
  if (info->done)
    return SVN_NO_ERROR;

  /* If the last rev we got for this path is less than CURRENT,
     then just return and don't fetch history for this path.
     The caller will get to this rev eventually or else reach
     the limit. */
  if (info->history_rev < current)
    return SVN_NO_ERROR;

  /* If the last rev we got for this path is equal to CURRENT
     then set *CHANGED to true and get the next history
     rev where this path was changed. */
  *changed = TRUE;
  return get_history(info, fs, strict, authz_read_func,
                     authz_read_baton, start, pool);
}

/* Return the next interesting revision in our list of HISTORIES. */
static svn_revnum_t
next_history_rev(apr_array_header_t *histories)
{
  svn_revnum_t next_rev = SVN_INVALID_REVNUM;
  int i;

  for (i = 0; i < histories->nelts; ++i)
    {
      struct path_info *info = APR_ARRAY_IDX(histories, i,
                                             struct path_info *);
      if (info->done)
        continue;
      if (info->history_rev > next_rev)
        next_rev = info->history_rev;
    }

  return next_rev;
}

/* Set *DELETED_MERGEINFO_CATALOG and *ADDED_MERGEINFO_CATALOG to
   catalogs describing how mergeinfo values on paths (which are the
   keys of those catalogs) were changed in REV. */
/* ### TODO: This would make a *great*, useful public function,
   ### svn_repos_fs_mergeinfo_changed()!  -- cmpilato  */
static svn_error_t *
fs_mergeinfo_changed(svn_mergeinfo_catalog_t *deleted_mergeinfo_catalog,
                     svn_mergeinfo_catalog_t *added_mergeinfo_catalog,
                     svn_fs_t *fs,
                     svn_revnum_t rev,
                     apr_pool_t *pool)

{
  apr_hash_t *changes;
  svn_fs_root_t *root;
  apr_pool_t *subpool = NULL, *iterpool;
  apr_hash_index_t *hi;

  /* Initialize return variables. */
  *deleted_mergeinfo_catalog = apr_hash_make(pool);
  *added_mergeinfo_catalog = apr_hash_make(pool);

  /* Revision 0 has no mergeinfo and no mergeinfo changes. */
  if (rev == 0)
    return SVN_NO_ERROR;

  subpool = svn_pool_create(pool);

  /* We're going to use the changed-paths information for REV to
     narrow down our search. */
  SVN_ERR(svn_fs_revision_root(&root, fs, rev, subpool));
  SVN_ERR(svn_fs_paths_changed2(&changes, root, subpool));

  /* No changed paths?  We're done. */
  if (apr_hash_count(changes) == 0)
    {
      svn_pool_destroy(subpool);
      return SVN_NO_ERROR;
    }

  /* Loop over changes, looking for anything that might carry an
     svn:mergeinfo change and is one of our paths of interest, or a
     child or [grand]parent directory thereof. */
  iterpool = svn_pool_create(subpool);
  for (hi = apr_hash_first(pool, changes); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_fs_path_change2_t *change;
      const char *changed_path, *base_path = NULL;
      svn_revnum_t base_rev = SVN_INVALID_REVNUM;
      svn_fs_root_t *base_root = NULL;
      svn_string_t *prev_mergeinfo_value = NULL, *mergeinfo_value;

      svn_pool_clear(iterpool);

      /* KEY will be the path, VAL the change. */
      apr_hash_this(hi, &key, NULL, &val);
      changed_path = key;
      change = val;

      /* If there was no property change on this item, ignore it. */
      if (! change->prop_mod)
        continue;

      switch (change->change_kind)
        {

        /* ### TODO: Can the add, replace, and modify cases be joined
           ### together to all use svn_repos__prev_location()?  The
           ### difference would be the fallback case (path/rev-1 for
           ### modifies, NULL otherwise).  -- cmpilato  */

        /* If the path was added or replaced, see if it was created via
           copy.  If so, that will tell us where its previous location
           was.  If not, there's no previous location to examine.  */
        case svn_fs_path_change_add:
        case svn_fs_path_change_replace:
          {
            const char *copyfrom_path;
            svn_revnum_t copyfrom_rev;

            SVN_ERR(svn_fs_copied_from(&copyfrom_rev, &copyfrom_path,
                                       root, changed_path, iterpool));
            if (copyfrom_path && SVN_IS_VALID_REVNUM(copyfrom_rev))
              {
                base_path = apr_pstrdup(subpool, copyfrom_path);
                base_rev = copyfrom_rev;
              }
            break;
          }

        /* If the path was merely modified, see if its previous
           location was affected by a copy which happened in this
           revision before assuming it holds the same path it did the
           previous revision. */
        case svn_fs_path_change_modify:
          {
            svn_revnum_t appeared_rev;

            SVN_ERR(svn_repos__prev_location(&appeared_rev, &base_path,
                                             &base_rev, fs, rev,
                                             changed_path, iterpool));

            /* If this path isn't the result of a copy that occurred
               in this revision, we can find the previous version of
               it in REV - 1 at the same path. */
            if (! (base_path && SVN_IS_VALID_REVNUM(base_rev)
                   && (appeared_rev == rev)))
              {
                base_path = changed_path;
                base_rev = rev - 1;
              }
            break;
          }

        /* We don't care about any of the other cases. */
        case svn_fs_path_change_delete:
        case svn_fs_path_change_reset:
        default:
          continue;
        }

      /* If there was a base location, fetch its mergeinfo property value. */
      if (base_path && SVN_IS_VALID_REVNUM(base_rev))
        {
          SVN_ERR(svn_fs_revision_root(&base_root, fs, base_rev, iterpool));
          SVN_ERR(svn_fs_node_prop(&prev_mergeinfo_value, base_root, base_path,
                                   SVN_PROP_MERGEINFO, iterpool));
        }

      /* Now fetch the current (as of REV) mergeinfo property value. */
      SVN_ERR(svn_fs_node_prop(&mergeinfo_value, root, changed_path,
                               SVN_PROP_MERGEINFO, iterpool));

      /* No mergeinfo on either the new or previous location?  Just
         skip it.  (If there *was* a change, it would have been in
         inherited mergeinfo only, which should be picked up by the
         iteration of this loop that finds the parent paths that
         really got changed.)  */
      if (! (mergeinfo_value || prev_mergeinfo_value))
        continue;

      /* If mergeinfo was explicitly added or removed on this path, we
         need to check to see if that was a real semantic change of
         meaning.  So, fill in the "missing" mergeinfo value with the
         inherited mergeinfo for that path/revision.  */
      if (prev_mergeinfo_value && (! mergeinfo_value))
        {
          apr_array_header_t *query_paths =
            apr_array_make(iterpool, 1, sizeof(const char *));
          svn_mergeinfo_t tmp_mergeinfo;
          svn_mergeinfo_catalog_t tmp_catalog;

          APR_ARRAY_PUSH(query_paths, const char *) = changed_path;
          SVN_ERR(svn_fs_get_mergeinfo(&tmp_catalog, root,
                                       query_paths, svn_mergeinfo_inherited,
                                       FALSE, iterpool));
          tmp_mergeinfo = apr_hash_get(tmp_catalog, changed_path,
                                        APR_HASH_KEY_STRING);
          if (tmp_mergeinfo)
            SVN_ERR(svn_mergeinfo_to_string(&mergeinfo_value,
                                            tmp_mergeinfo,
                                            iterpool));
        }
      else if (mergeinfo_value && (! prev_mergeinfo_value)
               && base_path && SVN_IS_VALID_REVNUM(base_rev))
        {
          apr_array_header_t *query_paths =
            apr_array_make(iterpool, 1, sizeof(const char *));
          svn_mergeinfo_t tmp_mergeinfo;
          svn_mergeinfo_catalog_t tmp_catalog;

          APR_ARRAY_PUSH(query_paths, const char *) = base_path;
          SVN_ERR(svn_fs_get_mergeinfo(&tmp_catalog, base_root,
                                       query_paths, svn_mergeinfo_inherited,
                                       FALSE, iterpool));
          tmp_mergeinfo = apr_hash_get(tmp_catalog, base_path,
                                        APR_HASH_KEY_STRING);
          if (tmp_mergeinfo)
            SVN_ERR(svn_mergeinfo_to_string(&prev_mergeinfo_value,
                                            tmp_mergeinfo,
                                            iterpool));
        }

      /* If the old and new mergeinfo differ in any way, store the
         before and after mergeinfo values in our return hashes. */
      if ((prev_mergeinfo_value && (! mergeinfo_value))
          || ((! prev_mergeinfo_value) && mergeinfo_value)
          || (prev_mergeinfo_value && mergeinfo_value
              && (! svn_string_compare(mergeinfo_value,
                                       prev_mergeinfo_value))))
        {
          svn_mergeinfo_t prev_mergeinfo = NULL, mergeinfo = NULL;
          svn_mergeinfo_t deleted, added;
          const char *hash_path;

          if (mergeinfo_value)
            SVN_ERR(svn_mergeinfo_parse(&mergeinfo,
                                        mergeinfo_value->data, iterpool));
          if (prev_mergeinfo_value)
            SVN_ERR(svn_mergeinfo_parse(&prev_mergeinfo,
                                        prev_mergeinfo_value->data, iterpool));
          SVN_ERR(svn_mergeinfo_diff(&deleted, &added, prev_mergeinfo,
                                     mergeinfo, FALSE, iterpool));

          /* Toss interesting stuff into our return catalogs. */
          hash_path = apr_pstrdup(pool, changed_path);
          apr_hash_set(*deleted_mergeinfo_catalog, hash_path,
                       APR_HASH_KEY_STRING, svn_mergeinfo_dup(deleted, pool));
          apr_hash_set(*added_mergeinfo_catalog, hash_path,
                       APR_HASH_KEY_STRING, svn_mergeinfo_dup(added, pool));
        }
    }
  /* ### UNNECESSARY ###: svn_pool_destroy(iterpool); */
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


/* Determine what (if any) mergeinfo for PATHS was modified in
   revision REV, returning the diffs as a single combined rangelist in
   *COMBINED_MERGEINFO.  Use POOL for all allocations. */
static svn_error_t *
get_combined_mergeinfo_changes(svn_mergeinfo_t *combined_mergeinfo,
                               svn_fs_t *fs,
                               const apr_array_header_t *paths,
                               svn_revnum_t rev,
                               apr_pool_t *pool)
{
  svn_mergeinfo_catalog_t added_mergeinfo_catalog, deleted_mergeinfo_catalog;
  apr_hash_index_t *hi;
  svn_fs_root_t *root;
  apr_pool_t *subpool, *iterpool;
  int i;

  /* Initialize return value. */
  *combined_mergeinfo = apr_hash_make(pool);

  /* If we're asking about revision 0, there's no mergeinfo to be found. */
  if (rev == 0)
    return SVN_NO_ERROR;

  /* No paths?  No mergeinfo. */
  if (! paths->nelts)
    return SVN_NO_ERROR;

  /* Create a work subpool and get a root for REV. */
  subpool = svn_pool_create(pool);
  SVN_ERR(svn_fs_revision_root(&root, fs, rev, subpool));

  /* Fetch the mergeinfo changes for REV. */
  SVN_ERR(fs_mergeinfo_changed(&deleted_mergeinfo_catalog,
                               &added_mergeinfo_catalog,
                               fs, rev, subpool));

  /* Check our PATHS for any changes to their inherited mergeinfo.
     (We deal with changes to mergeinfo directly *on* the paths in the
     following loop.)  */
  iterpool = svn_pool_create(subpool);
  for (i = 0; i < paths->nelts; i++)
    {
      const char *path = APR_ARRAY_IDX(paths, i, const char *);
      const char *prev_path;
      svn_revnum_t appeared_rev, prev_rev;
      svn_fs_root_t *prev_root;
      svn_mergeinfo_catalog_t catalog;
      svn_mergeinfo_t prev_mergeinfo, mergeinfo, deleted, added;
      apr_array_header_t *query_paths;
      svn_error_t *err;

      svn_pool_clear(iterpool);

      /* If this path is represented in the changed-mergeinfo hashes,
         we'll deal with it in the loop below. */
      if (apr_hash_get(deleted_mergeinfo_catalog, path, APR_HASH_KEY_STRING))
        continue;

      /* Figure out what path/rev to compare against.  Ignore
         not-found errors returned by the filesystem.  */
      err = svn_repos__prev_location(&appeared_rev, &prev_path, &prev_rev,
                                     fs, rev, path, iterpool);
      if (err && (err->apr_err == SVN_ERR_FS_NOT_FOUND ||
                  err->apr_err == SVN_ERR_FS_NOT_DIRECTORY))
        {
          svn_error_clear(err);
          err = SVN_NO_ERROR;
          continue;
        }
      SVN_ERR(err);

      /* If this path isn't the result of a copy that occurred in this
         revision, we can find the previous version of it in REV - 1
         at the same path. */
      if (! (prev_path && SVN_IS_VALID_REVNUM(prev_rev)
             && (appeared_rev == rev)))
        {
          prev_path = path;
          prev_rev = rev - 1;
        }

      /* Fetch the previous mergeinfo (including inherited stuff) for
         this path.  Ignore not-found errors returned by the
         filesystem.  */
      SVN_ERR(svn_fs_revision_root(&prev_root, fs, prev_rev, iterpool));
      query_paths = apr_array_make(iterpool, 1, sizeof(const char *));
      APR_ARRAY_PUSH(query_paths, const char *) = prev_path;
      err = svn_fs_get_mergeinfo(&catalog, prev_root, query_paths,
                                 svn_mergeinfo_inherited, FALSE, iterpool);
      if (err && (err->apr_err == SVN_ERR_FS_NOT_FOUND ||
                  err->apr_err == SVN_ERR_FS_NOT_DIRECTORY))
        {
          svn_error_clear(err);
          err = SVN_NO_ERROR;
          continue;
        }
      SVN_ERR(err);
      prev_mergeinfo = apr_hash_get(catalog, prev_path, APR_HASH_KEY_STRING);

      /* Fetch the current mergeinfo (as of REV, and including
         inherited stuff) for this path. */
      APR_ARRAY_IDX(query_paths, 0, const char *) = path;
      SVN_ERR(svn_fs_get_mergeinfo(&catalog, root, query_paths,
                                   svn_mergeinfo_inherited, FALSE, iterpool));
      mergeinfo = apr_hash_get(catalog, path, APR_HASH_KEY_STRING);

      /* Compare, constrast, and combine the results. */
      SVN_ERR(svn_mergeinfo_diff(&deleted, &added, prev_mergeinfo,
                                 mergeinfo, FALSE, iterpool));
      mergeinfo = deleted;
      SVN_ERR(svn_mergeinfo_merge(mergeinfo, added, iterpool));
      SVN_ERR(svn_mergeinfo_merge(*combined_mergeinfo,
                                  svn_mergeinfo_dup(mergeinfo, pool),
                                  pool));
    }
  svn_pool_destroy(iterpool);

  /* Merge all the mergeinfos which are, or are children of, one of
     our paths of interest into one giant delta mergeinfo.  */
  for (hi = apr_hash_first(NULL, added_mergeinfo_catalog);
       hi; hi = apr_hash_next(hi))
    {
      const void *key;
      apr_ssize_t klen;
      void *val;
      const char *changed_path;
      svn_mergeinfo_t added_mergeinfo, deleted_mergeinfo;

      /* The path is the key, the mergeinfo delta is the value. */
      apr_hash_this(hi, &key, &klen, &val);
      changed_path = key;
      added_mergeinfo = val;

      for (i = 0; i < paths->nelts; i++)
        {
          const char *path = APR_ARRAY_IDX(paths, i, const char *);
          if (! svn_path_is_ancestor(path, changed_path))
            continue;
          deleted_mergeinfo =
            apr_hash_get(deleted_mergeinfo_catalog, key, klen);
          SVN_ERR(svn_mergeinfo_merge(*combined_mergeinfo,
                                      svn_mergeinfo_dup(deleted_mergeinfo,
                                                        pool),
                                      pool));
          SVN_ERR(svn_mergeinfo_merge(*combined_mergeinfo,
                                      svn_mergeinfo_dup(added_mergeinfo,
                                                        pool),
                                      pool));
          break;
        }
    }
  svn_pool_clear(subpool);
  return SVN_NO_ERROR;
}


/* Fill LOG_ENTRY with history information in FS at REV. */
static svn_error_t *
fill_log_entry(svn_log_entry_t *log_entry,
               svn_revnum_t rev,
               svn_fs_t *fs,
               svn_boolean_t discover_changed_paths,
               const apr_array_header_t *revprops,
               svn_repos_authz_func_t authz_read_func,
               void *authz_read_baton,
               apr_pool_t *pool)
{
  apr_hash_t *r_props, *changed_paths = NULL;
  svn_boolean_t get_revprops = TRUE, censor_revprops = FALSE;

  /* Discover changed paths if the user requested them
     or if we need to check that they are readable. */
  if ((rev > 0)
      && (authz_read_func || discover_changed_paths))
    {
      svn_fs_root_t *newroot;
      svn_error_t *patherr;

      SVN_ERR(svn_fs_revision_root(&newroot, fs, rev, pool));
      patherr = detect_changed(&changed_paths,
                               newroot, fs,
                               authz_read_func, authz_read_baton,
                               pool);

      if (patherr
          && patherr->apr_err == SVN_ERR_AUTHZ_UNREADABLE)
        {
          /* All changed-paths are unreadable, so clear all fields. */
          svn_error_clear(patherr);
          changed_paths = NULL;
          get_revprops = FALSE;
        }
      else if (patherr
               && patherr->apr_err == SVN_ERR_AUTHZ_PARTIALLY_READABLE)
        {
          /* At least one changed-path was unreadable, so censor all
             but author and date.  (The unreadable paths are already
             missing from the hash.) */
          svn_error_clear(patherr);
          censor_revprops = TRUE;
        }
      else if (patherr)
        return patherr;

      /* It may be the case that an authz func was passed in, but
         the user still doesn't want to see any changed-paths. */
      if (! discover_changed_paths)
        changed_paths = NULL;
    }

  if (get_revprops)
    {
      /* User is allowed to see at least some revprops. */
      SVN_ERR(svn_fs_revision_proplist(&r_props, fs, rev, pool));
      if (revprops == NULL)
        {
          /* Requested all revprops... */
          if (censor_revprops)
            {
              /* ... but we can only return author/date. */
              log_entry->revprops = apr_hash_make(pool);
              apr_hash_set(log_entry->revprops, SVN_PROP_REVISION_AUTHOR,
                           APR_HASH_KEY_STRING,
                           apr_hash_get(r_props, SVN_PROP_REVISION_AUTHOR,
                                        APR_HASH_KEY_STRING));
              apr_hash_set(log_entry->revprops, SVN_PROP_REVISION_DATE,
                           APR_HASH_KEY_STRING,
                           apr_hash_get(r_props, SVN_PROP_REVISION_DATE,
                                        APR_HASH_KEY_STRING));
            }
          else
            /* ... so return all we got. */
            log_entry->revprops = r_props;
        }
      else
        {
          /* Requested only some revprops... */
          int i;
          for (i = 0; i < revprops->nelts; i++)
            {
              char *name = APR_ARRAY_IDX(revprops, i, char *);
              svn_string_t *value = apr_hash_get(r_props, name,
                                                 APR_HASH_KEY_STRING);
              if (censor_revprops
                  && !(strcmp(name, SVN_PROP_REVISION_AUTHOR) == 0
                       || strcmp(name, SVN_PROP_REVISION_DATE) == 0))
                /* ... but we can only return author/date. */
                continue;
              if (log_entry->revprops == NULL)
                log_entry->revprops = apr_hash_make(pool);
              apr_hash_set(log_entry->revprops, name,
                           APR_HASH_KEY_STRING, value);
            }
        }
    }

  log_entry->changed_paths = changed_paths;
  log_entry->changed_paths2 = changed_paths;
  log_entry->revision = rev;

  return SVN_NO_ERROR;
}

/* Send a log message for REV to RECEIVER with its RECEIVER_BATON.
 *
 * FS is used with REV to fetch the interesting history information,
 * such as changed paths, revprops, etc.
 *
 * The detect_changed function is used if either AUTHZ_READ_FUNC is
 * not NULL, or if DISCOVER_CHANGED_PATHS is TRUE.  See it for details.
 *
 * If DESCENDING_ORDER is true, send child messages in descending order.
 *
 * If REVPROPS is NULL, retrieve all revprops; else, retrieve only the
 * revprops named in the array (i.e. retrieve none if the array is empty).
 */
static svn_error_t *
send_log(svn_revnum_t rev,
         svn_fs_t *fs,
         svn_boolean_t discover_changed_paths,
         const apr_array_header_t *revprops,
         svn_boolean_t has_children,
         svn_log_entry_receiver_t receiver,
         void *receiver_baton,
         svn_repos_authz_func_t authz_read_func,
         void *authz_read_baton,
         apr_pool_t *pool)
{
  svn_log_entry_t *log_entry;

  log_entry = svn_log_entry_create(pool);
  SVN_ERR(fill_log_entry(log_entry, rev, fs, discover_changed_paths,
                         revprops, authz_read_func, authz_read_baton,
                         pool));
  log_entry->has_children = has_children;

  /* Send the entry to the receiver. */
  return (*receiver)(receiver_baton, log_entry, pool);
}

/* This controls how many history objects we keep open.  For any targets
   over this number we have to open and close their histories as needed,
   which is CPU intensive, but keeps us from using an unbounded amount of
   memory. */
#define MAX_OPEN_HISTORIES 32

/* Get the histories for PATHS, and store them in *HISTORIES. */
static svn_error_t *
get_path_histories(apr_array_header_t **histories,
                   svn_fs_t *fs,
                   const apr_array_header_t *paths,
                   svn_revnum_t hist_start,
                   svn_revnum_t hist_end,
                   svn_boolean_t strict_node_history,
                   svn_repos_authz_func_t authz_read_func,
                   void *authz_read_baton,
                   apr_pool_t *pool)
{
  svn_fs_root_t *root;
  apr_pool_t *iterpool;
  int i;

  /* Create a history object for each path so we can walk through
     them all at the same time until we have all changes or LIMIT
     is reached.

     There is some pool fun going on due to the fact that we have
     to hold on to the old pool with the history before we can
     get the next history.
  */
  *histories = apr_array_make(pool, paths->nelts,
                              sizeof(struct path_info *));

  SVN_ERR(svn_fs_revision_root(&root, fs, hist_end, pool));

  iterpool = svn_pool_create(pool);
  for (i = 0; i < paths->nelts; i++)
    {
      const char *this_path = APR_ARRAY_IDX(paths, i, const char *);
      struct path_info *info = apr_palloc(pool,
                                          sizeof(struct path_info));

      if (authz_read_func)
        {
          svn_boolean_t readable;

          svn_pool_clear(iterpool);

          SVN_ERR(authz_read_func(&readable, root, this_path,
                                  authz_read_baton, iterpool));
          if (! readable)
            return svn_error_create(SVN_ERR_AUTHZ_UNREADABLE, NULL, NULL);
        }

      info->path = svn_stringbuf_create(this_path, pool);
      info->done = FALSE;
      info->history_rev = hist_end;
      info->first_time = TRUE;

      if (i < MAX_OPEN_HISTORIES)
        {
          SVN_ERR(svn_fs_node_history(&info->hist, root, this_path, pool));
          info->newpool = svn_pool_create(pool);
          info->oldpool = svn_pool_create(pool);
        }
      else
        {
          info->hist = NULL;
          info->oldpool = NULL;
          info->newpool = NULL;
        }

      SVN_ERR(get_history(info, fs,
                          strict_node_history,
                          authz_read_func, authz_read_baton,
                          hist_start, pool));
      APR_ARRAY_PUSH(*histories, struct path_info *) = info;
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* Remove and return the first item from ARR. */
static void *
array_pop_front(apr_array_header_t *arr)
{
  void *item = arr->elts;

  if (apr_is_empty_array(arr))
    return NULL;

  arr->elts += arr->elt_size;
  arr->nelts -= 1;
  arr->nalloc -= 1;
  return item;
}

/* A struct which represents a single revision range, and the paths which
   have mergeinfo in that range. */
struct path_list_range
{
  apr_array_header_t *paths;
  svn_merge_range_t range;
};

/* A struct which represents "inverse mergeinfo", that is, instead of having
   a path->revision_range_list mapping, which is the way mergeinfo is commonly
   represented, this struct enables a revision_range_list,path tuple, where
   the paths can be accessed by revision. */
struct rangelist_path
{
  apr_array_header_t *rangelist;
  const char *path;
};

/* Comparator function for combine_mergeinfo_path_lists().  Sorts
   rangelist_path structs in increasing order based upon starting revision,
   then ending revision of the first element in the rangelist.

   This does not sort rangelists based upon subsequent elements, only the
   first range.  We'll sort any subsequent ranges in the correct order
   when they get bumped up to the front by removal of earlier ones, so we
   don't really have to sort them here.  See combine_mergeinfo_path_lists()
   for details. */
static int
compare_rangelist_paths(const void *a, const void *b)
{
  struct rangelist_path *rpa = *((struct rangelist_path **) a);
  struct rangelist_path *rpb = *((struct rangelist_path **) b);
  svn_merge_range_t *mra = APR_ARRAY_IDX(rpa->rangelist, 0,
                                         svn_merge_range_t *);
  svn_merge_range_t *mrb = APR_ARRAY_IDX(rpb->rangelist, 0,
                                         svn_merge_range_t *);

  if (mra->start < mrb->start)
    return -1;
  if (mra->start > mrb->start)
    return 1;
  if (mra->end < mrb->end)
    return -1;
  if (mra->end > mrb->end)
    return 1;

  return 0;
}

/* From MERGEINFO, return in *COMBINED_LIST, allocated in POOL, a list of
   'struct path_list_range's.  This list represents the rangelists in
   MERGEINFO and each path which has mergeinfo in that range.  */
static svn_error_t *
combine_mergeinfo_path_lists(apr_array_header_t **combined_list,
                             svn_mergeinfo_t mergeinfo,
                             apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_array_header_t *rangelist_paths;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Create a list of (revision range, path) tuples from MERGEINFO. */
  rangelist_paths = apr_array_make(subpool, apr_hash_count(mergeinfo),
                                   sizeof(struct rangelist_path *));
  for (hi = apr_hash_first(subpool, mergeinfo); hi;
       hi = apr_hash_next(hi))
    {
      int i;
      struct rangelist_path *rp = apr_palloc(subpool, sizeof(*rp));
      apr_hash_this(hi, (void *) &rp->path, NULL,
                    (void *) &rp->rangelist);
      APR_ARRAY_PUSH(rangelist_paths, struct rangelist_path *) = rp;

      /* We need to make local copies of the rangelist, since we will be
         modifying it, below. */
      rp->rangelist = svn_rangelist_dup(rp->rangelist, subpool);

      /* Make all of the rangelists inclusive, both start and end. */
      for (i = 0; i < rp->rangelist->nelts; i++)
        APR_ARRAY_IDX(rp->rangelist, i, svn_merge_range_t *)->start += 1;
    }

  /* Loop over the (revision range, path) tuples, chopping them into
     (revision range, paths) tuples, and appending those to the output
     list. */
  *combined_list = apr_array_make(pool, 0, sizeof(struct path_list_range *));
  while (rangelist_paths->nelts > 1)
    {
      svn_revnum_t youngest, next_youngest, tail, youngest_end;
      struct path_list_range *plr;
      struct rangelist_path *rp;
      int num_revs;
      int i;

      /* First, sort the list such that the start revision of the first
         revision arrays are sorted. */
      qsort(rangelist_paths->elts, rangelist_paths->nelts,
            rangelist_paths->elt_size, compare_rangelist_paths);

      /* Next, find the number of revision ranges which start with the same
         revision. */
      rp = APR_ARRAY_IDX(rangelist_paths, 0, struct rangelist_path *);
      youngest =
        APR_ARRAY_IDX(rp->rangelist, 0, struct svn_merge_range_t *)->start;
      next_youngest = youngest;
      for (num_revs = 1; next_youngest == youngest; num_revs++)
        {
          if (num_revs == rangelist_paths->nelts)
            {
              num_revs += 1;
              break;
            }
          rp = APR_ARRAY_IDX(rangelist_paths, num_revs,
                             struct rangelist_path *);
          next_youngest = APR_ARRAY_IDX(rp->rangelist, 0,
                                        struct svn_merge_range_t *)->start;
        }
      num_revs -= 1;

      /* The start of the new range will be YOUNGEST, and we now find the end
         of the new range, which should be either one less than the next
         earliest start of a rangelist, or the end of the first rangelist. */
      youngest_end =
        APR_ARRAY_IDX(APR_ARRAY_IDX(rangelist_paths, 0,
                                    struct rangelist_path *)->rangelist,
                      0, svn_merge_range_t *)->end;
      if ( (next_youngest == youngest) || (youngest_end < next_youngest) )
        tail = youngest_end;
      else
        tail = next_youngest - 1;

      /* Insert the (earliest, tail) tuple into the output list, along with
         a list of paths which match it. */
      plr = apr_palloc(pool, sizeof(*plr));
      plr->range.start = youngest;
      plr->range.end = tail;
      plr->paths = apr_array_make(pool, num_revs, sizeof(const char *));
      for (i = 0; i < num_revs; i++)
        APR_ARRAY_PUSH(plr->paths, const char *) =
          APR_ARRAY_IDX(rangelist_paths, i, struct rangelist_path *)->path;
      APR_ARRAY_PUSH(*combined_list, struct path_list_range *) = plr;

      /* Now, check to see which (rangelist path) combinations we can remove,
         and do so. */
      for (i = 0; i < num_revs; i++)
        {
          svn_merge_range_t *range;
          rp = APR_ARRAY_IDX(rangelist_paths, i, struct rangelist_path *);
          range = APR_ARRAY_IDX(rp->rangelist, 0, svn_merge_range_t *);

          /* Set the start of the range to beyond the end of the range we
             just built.  If the range is now "inverted", we can get pop it
             off the list. */
          range->start = tail + 1;
          if (range->start > range->end)
            {
              if (rp->rangelist->nelts == 1)
                {
                  /* The range is the only on its list, so we should remove
                     the entire rangelist_path, adjusting our loop control
                     variables appropriately. */
                  array_pop_front(rangelist_paths);
                  i--;
                  num_revs--;
                }
              else
                {
                  /* We have more than one range on the list, so just remove
                     the first one. */
                  array_pop_front(rp->rangelist);
                }
            }
        }
    }

  /* Finally, add the last remaining (revision range, path) to the output
     list. */
  if (rangelist_paths->nelts > 0)
    {
      struct rangelist_path *first_rp =
        APR_ARRAY_IDX(rangelist_paths, 0, struct rangelist_path *);
      while (first_rp->rangelist->nelts > 0)
        {
          struct path_list_range *plr = apr_palloc(pool, sizeof(*plr));

          plr->paths = apr_array_make(pool, 1, sizeof(const char *));
          APR_ARRAY_PUSH(plr->paths, const char *) = first_rp->path;
          plr->range = *APR_ARRAY_IDX(first_rp->rangelist, 0,
                                      svn_merge_range_t *);
          array_pop_front(first_rp->rangelist);
          APR_ARRAY_PUSH(*combined_list, struct path_list_range *) = plr;
        }
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Pity that C is so ... linear. */
static svn_error_t *
do_logs(svn_fs_t *fs,
        const apr_array_header_t *paths,
        svn_revnum_t hist_start,
        svn_revnum_t hist_end,
        int limit,
        svn_boolean_t discover_changed_paths,
        svn_boolean_t strict_node_history,
        svn_boolean_t include_merged_revisions,
        const apr_array_header_t *revprops,
        svn_boolean_t descending_order,
        svn_log_entry_receiver_t receiver,
        void *receiver_baton,
        svn_repos_authz_func_t authz_read_func,
        void *authz_read_baton,
        apr_pool_t *pool);


/* Examine the combined mergeinfo for revision REV in FS (as collected
   by examining paths of interest to a log operation), and determine
   which revisions to report as having been merged via the commit
   resulting in REV.

   Other parameters are as described by do_logs(), around which this
   is a recursion wrapper. */
static svn_error_t *
handle_merged_revisions(svn_revnum_t rev,
                        svn_fs_t *fs,
                        svn_mergeinfo_t mergeinfo,
                        svn_boolean_t discover_changed_paths,
                        svn_boolean_t strict_node_history,
                        const apr_array_header_t *revprops,
                        svn_log_entry_receiver_t receiver,
                        void *receiver_baton,
                        svn_repos_authz_func_t authz_read_func,
                        void *authz_read_baton,
                        apr_pool_t *pool)
{
  apr_array_header_t *combined_list;
  svn_log_entry_t *empty_log_entry;
  apr_pool_t *iterpool;
  int i;

  if (apr_hash_count(mergeinfo) == 0)
    return SVN_NO_ERROR;

  SVN_ERR(combine_mergeinfo_path_lists(&combined_list, mergeinfo, pool));

  /* Because the combined_lists are ordered youngest to oldest,
     iterate over them in reverse. */
  iterpool = svn_pool_create(pool);
  for (i = combined_list->nelts - 1; i >= 0; i--)
    {
      svn_error_t *err;
      struct path_list_range *pl_range
        = APR_ARRAY_IDX(combined_list, i, struct path_list_range *);

      svn_pool_clear(iterpool);
      err = do_logs(fs, pl_range->paths, pl_range->range.start,
                    pl_range->range.end, 0, discover_changed_paths,
                    strict_node_history, TRUE, revprops, TRUE,
                    receiver, receiver_baton, authz_read_func,
                    authz_read_baton, iterpool);
      if (err && (err->apr_err == SVN_ERR_FS_NOT_FOUND ||
                  err->apr_err == SVN_ERR_FS_NOT_DIRECTORY ||
                  err->apr_err == SVN_ERR_FS_NO_SUCH_REVISION))
        {
          svn_error_clear(err);
          continue;
        }
      SVN_ERR(err);
    }
  svn_pool_destroy(iterpool);

  /* Send the empty revision.  */
  empty_log_entry = svn_log_entry_create(pool);
  empty_log_entry->revision = SVN_INVALID_REVNUM;
  return (*receiver)(receiver_baton, empty_log_entry, pool);
}

/* Find logs for PATHS from HIST_START to HIST_END in FS, and invoke
   RECEIVER with RECEIVER_BATON on them.  If DESCENDING_ORDER is TRUE, send
   the logs back as we find them, else buffer the logs and send them back
   in youngest->oldest order.

   Other parameters are the same as svn_repos_get_logs4().
 */
static svn_error_t *
do_logs(svn_fs_t *fs,
        const apr_array_header_t *paths,
        svn_revnum_t hist_start,
        svn_revnum_t hist_end,
        int limit,
        svn_boolean_t discover_changed_paths,
        svn_boolean_t strict_node_history,
        svn_boolean_t include_merged_revisions,
        const apr_array_header_t *revprops,
        svn_boolean_t descending_order,
        svn_log_entry_receiver_t receiver,
        void *receiver_baton,
        svn_repos_authz_func_t authz_read_func,
        void *authz_read_baton,
        apr_pool_t *pool)
{
  apr_pool_t *iterpool;
  apr_array_header_t *revs = NULL;
  apr_hash_t *rev_mergeinfo = NULL;
  svn_revnum_t current;
  apr_array_header_t *histories;
  svn_boolean_t any_histories_left = TRUE;
  int send_count = 0;
  int i;

  /* We have a list of paths and a revision range.  But we don't care
     about all the revisions in the range -- only the ones in which
     one of our paths was changed.  So let's go figure out which
     revisions contain real changes to at least one of our paths.  */
  SVN_ERR(get_path_histories(&histories, fs, paths, hist_start, hist_end,
                             strict_node_history, authz_read_func,
                             authz_read_baton, pool));

  /* Loop through all the revisions in the range and add any
     where a path was changed to the array, or if they wanted
     history in reverse order just send it to them right away. */
  iterpool = svn_pool_create(pool);
  for (current = hist_end;
       any_histories_left;
       current = next_history_rev(histories))
    {
      svn_boolean_t changed = FALSE;
      any_histories_left = FALSE;
      svn_pool_clear(iterpool);

      for (i = 0; i < histories->nelts; i++)
        {
          struct path_info *info = APR_ARRAY_IDX(histories, i,
                                                 struct path_info *);

          /* Check history for this path in current rev. */
          SVN_ERR(check_history(&changed, info, fs, current,
                                strict_node_history, authz_read_func,
                                authz_read_baton, hist_start, pool));
          if (! info->done)
            any_histories_left = TRUE;
        }

      /* If any of the paths changed in this rev then add or send it. */
      if (changed)
        {
          svn_mergeinfo_t mergeinfo = NULL;
          svn_boolean_t has_children = FALSE;

          /* If we're including merged revisions, we need to calculate
             the mergeinfo deltas committed in this revision to our
             various paths. */
          if (include_merged_revisions)
            {
              apr_array_header_t *cur_paths =
                apr_array_make(iterpool, paths->nelts, sizeof(const char *));

              /* Get the current paths of our history objects so we can
                 query mergeinfo. */
              /* ### TODO: Should this be ignoring depleted history items? */
              for (i = 0; i < histories->nelts; i++)
                {
                  struct path_info *info = APR_ARRAY_IDX(histories, i,
                                                         struct path_info *);
                  APR_ARRAY_PUSH(cur_paths, const char *) = info->path->data;
                }
              SVN_ERR(get_combined_mergeinfo_changes(&mergeinfo, fs, cur_paths,
                                                     current, iterpool));
              has_children = (apr_hash_count(mergeinfo) > 0);
            }

          /* If our caller wants logs in descending order, we can send
             'em now (because that's the order we're crawling history
             in anyway). */
          if (descending_order)
            {
              SVN_ERR(send_log(current, fs, discover_changed_paths,
                               revprops, has_children, receiver, receiver_baton,
                               authz_read_func, authz_read_baton, iterpool));
              if (has_children)
                {
                  SVN_ERR(handle_merged_revisions(current, fs, mergeinfo,
                                                  discover_changed_paths,
                                                  strict_node_history, revprops,
                                                  receiver, receiver_baton,
                                                  authz_read_func,
                                                  authz_read_baton,
                                                  iterpool));
                }
              if (limit && ++send_count >= limit)
                break;
            }
          /* Otherwise, the caller wanted logs in ascending order, so
             we have to buffer up a list of revs and (if doing
             mergeinfo) a hash of related mergeinfo deltas, and
             process them later. */
          else
            {
              if (! revs)
                revs = apr_array_make(pool, 64, sizeof(svn_revnum_t));
              APR_ARRAY_PUSH(revs, svn_revnum_t) = current;

              if (mergeinfo)
                {
                  svn_revnum_t *cur_rev = apr_palloc(pool, sizeof(*cur_rev));
                  *cur_rev = current;
                  if (! rev_mergeinfo)
                    rev_mergeinfo = apr_hash_make(pool);
                  apr_hash_set(rev_mergeinfo, cur_rev, sizeof(*cur_rev),
                               svn_mergeinfo_dup(mergeinfo, pool));
                }
            }
        }
    }
  svn_pool_destroy(iterpool);

  if (revs)
    {
      /* Work loop for processing the revisions we found since they wanted
         history in forward order. */
      iterpool = svn_pool_create(pool);
      for (i = 0; i < revs->nelts; ++i)
        {
          svn_mergeinfo_t mergeinfo;
          svn_boolean_t has_children = FALSE;

          svn_pool_clear(iterpool);
          current = APR_ARRAY_IDX(revs, revs->nelts - i - 1, svn_revnum_t);

          /* If we've got a hash of revision mergeinfo (which can only
             happen if INCLUDE_MERGED_REVISIONS was set), we check to
             see if this revision is one which merged in other
             revisions we need to handle recursively. */
          if (rev_mergeinfo)
            {
              mergeinfo = apr_hash_get(rev_mergeinfo, &current,
                                       sizeof(svn_revnum_t));
              has_children = (apr_hash_count(mergeinfo) > 0);
            }

          SVN_ERR(send_log(current, fs,
                           discover_changed_paths, revprops, has_children,
                           receiver, receiver_baton, authz_read_func,
                           authz_read_baton, iterpool));
          if (has_children)
            {
              SVN_ERR(handle_merged_revisions(current, fs, mergeinfo,
                                              discover_changed_paths,
                                              strict_node_history, revprops,
                                              receiver, receiver_baton,
                                              authz_read_func,
                                              authz_read_baton,
                                              iterpool));
            }
          if (limit && i + 1 >= limit)
            break;
        }
      svn_pool_destroy(iterpool);
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_repos_get_logs4(svn_repos_t *repos,
                    const apr_array_header_t *paths,
                    svn_revnum_t start,
                    svn_revnum_t end,
                    int limit,
                    svn_boolean_t discover_changed_paths,
                    svn_boolean_t strict_node_history,
                    svn_boolean_t include_merged_revisions,
                    const apr_array_header_t *revprops,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_log_entry_receiver_t receiver,
                    void *receiver_baton,
                    apr_pool_t *pool)
{
  svn_revnum_t head = SVN_INVALID_REVNUM;
  svn_fs_t *fs = repos->fs;
  svn_boolean_t descending_order;

  /* Setup log range. */
  SVN_ERR(svn_fs_youngest_rev(&head, fs, pool));

  if (! SVN_IS_VALID_REVNUM(start))
    start = head;

  if (! SVN_IS_VALID_REVNUM(end))
    end = head;

  /* Check that revisions are sane before ever invoking receiver. */
  if (start > head)
    return svn_error_createf
      (SVN_ERR_FS_NO_SUCH_REVISION, 0,
       _("No such revision %ld"), start);
  if (end > head)
    return svn_error_createf
      (SVN_ERR_FS_NO_SUCH_REVISION, 0,
       _("No such revision %ld"), end);

  /* Ensure a youngest-to-oldest revision crawl ordering using our
     (possibly sanitized) range values. */
  descending_order = start >= end;
  if (descending_order)
    {
      svn_revnum_t tmp_rev = start;
      start = end;
      end = tmp_rev;
    }

  if (! paths)
    paths = apr_array_make(pool, 0, sizeof(const char *));

  /* If we're not including merged revisions, and we were given no
     paths or a single empty (or "/") path, then we can bypass a bunch
     of complexity because we already know in which revisions the root
     directory was changed -- all of them.  */
  if ((! include_merged_revisions)
      && ((! paths->nelts)
          || ((paths->nelts == 1)
              && (svn_path_is_empty(APR_ARRAY_IDX(paths, 0, const char *))
                  || (strcmp(APR_ARRAY_IDX(paths, 0, const char *),
                             "/") == 0)))))
    {
      int send_count = 0;
      int i;
      apr_pool_t *iterpool = svn_pool_create(pool);

      send_count = end - start + 1;
      if (limit && send_count > limit)
        send_count = limit;
      for (i = 0; i < send_count; ++i)
        {
          svn_revnum_t rev = start + i;

          svn_pool_clear(iterpool);

          if (descending_order)
            rev = end - i;
          SVN_ERR(send_log(rev, fs, discover_changed_paths, revprops, FALSE,
                           receiver, receiver_baton, authz_read_func,
                           authz_read_baton, iterpool));
        }
      svn_pool_destroy(iterpool);

      return SVN_NO_ERROR;
    }

  return do_logs(repos->fs, paths, start, end, limit,
                 discover_changed_paths, strict_node_history,
                 include_merged_revisions, revprops, descending_order,
                 receiver, receiver_baton,
                 authz_read_func, authz_read_baton, pool);
}
