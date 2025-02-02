/*
 * io.c:   shared file reading, writing, and probing code.
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



#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#endif

#ifndef APR_STATUS_IS_EPERM
#include <errno.h>
#ifdef EPERM
#define APR_STATUS_IS_EPERM(s)   ((s) == EPERM)
#else
#define APR_STATUS_IS_EPERM(s)   (0)
#endif
#endif

#include <apr_lib.h>
#include <apr_pools.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_general.h>
#include <apr_strings.h>
#include <apr_portable.h>
#include <apr_md5.h>

#include "svn_types.h"
#include "svn_path.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_io.h"
#include "svn_pools.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_private_config.h"

#define SVN_SLEEP_ENV_VAR "SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_SLEEP_FOR_TIMESTAMPS"

/*
  Windows is 'aided' by a number of types of applications that
  follow other applications around and open up files they have
  changed for various reasons (the most intrusive are virus
  scanners).  So, if one of these other apps has glommed onto
  our file we may get an 'access denied' error.

  This retry loop does not completely solve the problem (who
  knows how long the other app is going to hold onto it for), but
  goes a long way towards minimizing it.  It is not an infinite
  loop because there might really be an error.

  Another reason for retrying delete operations on Windows
  is that they are asynchronous -- the file or directory is not
  actually deleted until the last handle to it is closed.  The
  retry loop cannot completely solve this problem either, but can
  help mitigate it.
*/
#ifdef WIN32
#define WIN32_RETRY_LOOP(err, expr)                                        \
  do                                                                       \
    {                                                                      \
      apr_status_t os_err = APR_TO_OS_ERROR(err);                       \
      int sleep_count = 1000;                                              \
      int retries;                                                         \
      for (retries = 0;                                                    \
           retries < 100 && (os_err == ERROR_ACCESS_DENIED                 \
                             || os_err == ERROR_SHARING_VIOLATION          \
                             || os_err == ERROR_DIR_NOT_EMPTY);            \
           ++retries, os_err = APR_TO_OS_ERROR(err))                    \
        {                                                                  \
          apr_sleep(sleep_count);                                       \
          if (sleep_count < 128000)                                        \
            sleep_count *= 2;                                              \
          (err) = (expr);                                                  \
        }                                                                  \
    }                                                                      \
  while (0)
#else
#define WIN32_RETRY_LOOP(err, expr) ((void)0)
#endif

/* Local wrapper of svn_path_cstring_to_utf8() that does no copying on
 * operating systems where APR always uses utf-8 as native path format */
static svn_error_t *
cstring_to_utf8(const char **path_utf8,
                const char *path_apr,
                apr_pool_t *pool)
{
#if defined(WIN32) || defined(DARWIN)
  pool; /* Unused */
  *path_utf8 = path_apr;
  return SVN_NO_ERROR;
#else
  return svn_path_cstring_to_utf8(path_utf8, path_apr, pool);
#endif
}

/* Local wrapper of svn_path_cstring_from_utf8() that does no copying on
 * operating systems where APR always uses utf-8 as native path format */
static svn_error_t *
cstring_from_utf8(const char **path_apr,
                  const char *path_utf8,
                  apr_pool_t *pool)
{
#if defined(WIN32) || defined(DARWIN)
  pool; /* Unused */
  *path_apr = path_utf8;
  return SVN_NO_ERROR;
#else
  return svn_path_cstring_from_utf8(path_apr, path_utf8, pool);
#endif
}


/* Set *NAME_P to the UTF-8 representation of directory entry NAME.
 * NAME is in the the internal encoding used by APR; PARENT is in
 * UTF-8 and in internal (not local) style.
 *
 * Use PARENT only for generating an error string if the conversion
 * fails because NAME could not be represented in UTF-8.  In that
 * case, return a two-level error in which the outer error's message
 * mentions PARENT, but the inner error's message does not mention
 * NAME (except possibly in hex) since NAME may not be printable.
 * Such a compound error at least allows the user to go looking in the
 * right directory for the problem.
 *
 * If there is any other error, just return that error directly.
 *
 * If there is any error, the effect on *NAME_P is undefined.
 *
 * *NAME_P and NAME may refer to the same storage.
 */
static svn_error_t *
entry_name_to_utf8(const char **name_p,
                   const char *name,
                   const char *parent,
                   apr_pool_t *pool)
{
  svn_error_t *err = svn_path_cstring_to_utf8(name_p, name, pool);
  if (err && err->apr_err == APR_EINVAL)
    {
      return svn_error_createf(err->apr_err, err,
                               _("Error converting entry "
                                 "in directory '%s' to UTF-8"),
                               svn_path_local_style(parent, pool));
    }
  return err;
}



static void
map_apr_finfo_to_node_kind(svn_node_kind_t *kind,
                           svn_boolean_t *is_special,
                           apr_finfo_t *finfo)
{
  *is_special = FALSE;

  if (finfo->filetype == APR_REG)
    *kind = svn_node_file;
  else if (finfo->filetype == APR_DIR)
    *kind = svn_node_dir;
  else if (finfo->filetype == APR_LNK)
    {
      *is_special = TRUE;
      *kind = svn_node_file;
    }
  else
    *kind = svn_node_unknown;
}

/* Helper for svn_io_check_path() and svn_io_check_resolved_path();
   essentially the same semantics as those two, with the obvious
   interpretation for RESOLVE_SYMLINKS. */
static svn_error_t *
io_check_path(const char *path,
              svn_boolean_t resolve_symlinks,
              svn_boolean_t *is_special_p,
              svn_node_kind_t *kind,
              apr_pool_t *pool)
{
  apr_int32_t flags;
  apr_finfo_t finfo;
  apr_status_t apr_err;
  const char *path_apr;
  svn_boolean_t is_special = FALSE;

  if (path[0] == '\0')
    path = ".";

  /* Not using svn_io_stat() here because we want to check the
     apr_err return explicitly. */
  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  flags = resolve_symlinks ? APR_FINFO_MIN : (APR_FINFO_MIN | APR_FINFO_LINK);
  apr_err = apr_stat(&finfo, path_apr, flags, pool);

  if (APR_STATUS_IS_ENOENT(apr_err))
    *kind = svn_node_none;
  else if (APR_STATUS_IS_ENOTDIR(apr_err))
    *kind = svn_node_none;
  else if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't check path '%s'"),
                              svn_path_local_style(path, pool));
  else
    map_apr_finfo_to_node_kind(kind, &is_special, &finfo);

  *is_special_p = is_special;

  return SVN_NO_ERROR;
}


/* Wrapper for apr_file_open(). */
static apr_status_t
file_open(apr_file_t **f,
          const char *fname,
          apr_int32_t flag,
          apr_fileperms_t perm,
          svn_boolean_t retry_on_failure,
          apr_pool_t *pool)
{
  apr_status_t status = apr_file_open(f, fname, flag, perm, pool);

  if (retry_on_failure)
    {
      WIN32_RETRY_LOOP(status, apr_file_open(f, fname, flag, perm, pool));
    }
  return status;
}


svn_error_t *
svn_io_check_resolved_path(const char *path,
                           svn_node_kind_t *kind,
                           apr_pool_t *pool)
{
  svn_boolean_t ignored;
  return io_check_path(path, TRUE, &ignored, kind, pool);
}

svn_error_t *
svn_io_check_path(const char *path,
                  svn_node_kind_t *kind,
                  apr_pool_t *pool)
{
  svn_boolean_t ignored;
  return io_check_path(path, FALSE, &ignored, kind, pool);
}

svn_error_t *
svn_io_check_special_path(const char *path,
                          svn_node_kind_t *kind,
                          svn_boolean_t *is_special,
                          apr_pool_t *pool)
{
  return io_check_path(path, FALSE, is_special, kind, pool);
}

struct temp_file_cleanup_s
{
  apr_pool_t *pool;
  const char *name;
};


static apr_status_t
temp_file_plain_cleanup_handler(void *baton)
{
  struct  temp_file_cleanup_s *b = baton;
  apr_status_t apr_err = APR_SUCCESS;

  if (b->name)
    {
      apr_err = apr_file_remove(b->name, b->pool);
      WIN32_RETRY_LOOP(apr_err, apr_file_remove(b->name, b->pool));
    }

  return apr_err;
}


static apr_status_t
temp_file_child_cleanup_handler(void *baton)
{
  struct  temp_file_cleanup_s *b = baton;

  apr_pool_cleanup_kill(b->pool, b,
                        temp_file_plain_cleanup_handler);

  return APR_SUCCESS;
}


svn_error_t *
svn_io_open_uniquely_named(apr_file_t **file,
                           const char **unique_path,
                           const char *dirpath,
                           const char *filename,
                           const char *suffix,
                           svn_io_file_del_t delete_when,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  const char *path;
  unsigned int i;
  struct temp_file_cleanup_s *baton = NULL;

  SVN_ERR_ASSERT(file || unique_path);

  if (dirpath == NULL)
    SVN_ERR(svn_io_temp_dir(&dirpath, scratch_pool));
  if (filename == NULL)
    filename = "tempfile";
  if (suffix == NULL)
    suffix = ".tmp";

  path = svn_path_join(dirpath, filename, scratch_pool);

  if (delete_when == svn_io_file_del_on_pool_cleanup)
    {
      baton = apr_palloc(result_pool, sizeof(*baton));

      baton->pool = result_pool;
      baton->name = NULL;

      /* Because cleanups are run LIFO, we need to make sure to register
         our cleanup before the apr_file_close cleanup:

         On Windows, you can't remove an open file.
      */
      apr_pool_cleanup_register(result_pool, baton,
                                temp_file_plain_cleanup_handler,
                                temp_file_child_cleanup_handler);
    }

  for (i = 1; i <= 99999; i++)
    {
      const char *unique_name;
      const char *unique_name_apr;
      apr_file_t *try_file;
      apr_status_t apr_err;
      apr_int32_t flag = (APR_READ | APR_WRITE | APR_CREATE | APR_EXCL
                          | APR_BUFFERED | APR_BINARY);

      if (delete_when == svn_io_file_del_on_close)
        flag |= APR_DELONCLOSE;

      /* Special case the first attempt -- if we can avoid having a
         generated numeric portion at all, that's best.  So first we
         try with just the suffix; then future tries add a number
         before the suffix.  (A do-while loop could avoid the repeated
         conditional, but it's not worth the clarity loss.)

         If the first attempt fails, the first number will be "2".
         This is good, since "1" would misleadingly imply that
         the second attempt was actually the first... and if someone's
         got conflicts on their conflicts, we probably don't want to
         add to their confusion :-). */
      /* ### could alloc in scratch, then copy to result.. but "meh" */
      if (i == 1)
        unique_name = apr_psprintf(result_pool, "%s%s", path, suffix);
      else
        unique_name = apr_psprintf(result_pool, "%s.%u%s", path, i, suffix);

      /* Hmmm.  Ideally, we would append to a native-encoding buf
         before starting iteration, then convert back to UTF-8 for
         return. But I suppose that would make the appending code
         sensitive to i18n in a way it shouldn't be... Oh well. */
      /* ### could alloc in scratch, then copy to result in baton... */
      SVN_ERR(cstring_from_utf8(&unique_name_apr, unique_name, result_pool));

      apr_err = file_open(&try_file, unique_name_apr, flag,
                          APR_OS_DEFAULT, FALSE, result_pool);

      if (APR_STATUS_IS_EEXIST(apr_err))
        continue;
      else if (apr_err)
        {
          /* On Win32, CreateFile fails with an "Access Denied" error
             code, rather than "File Already Exists", if the colliding
             name belongs to a directory. */
          if (APR_STATUS_IS_EACCES(apr_err))
            {
              apr_finfo_t finfo;
              apr_status_t apr_err_2 = apr_stat(&finfo, unique_name_apr,
                                                APR_FINFO_TYPE, scratch_pool);

              if (!apr_err_2 && finfo.filetype == APR_DIR)
                continue;

#if WIN32
              apr_err_2 = APR_TO_OS_ERROR(apr_err);

              if (apr_err_2 == ERROR_ACCESS_DENIED ||
                  apr_err_2 == ERROR_SHARING_VIOLATION)
                {
                  /* The file is in use by another process or is hidden;
                     create a new name, but don't do this 99999 times in
                     case the folder is not writable */
                  i += 797;
                  continue;
                }
#endif

              /* Else fall through and return the original error. */
            }

          if (file) *file = NULL;
          if (unique_path) *unique_path = NULL;
          return svn_error_wrap_apr(apr_err, _("Can't open '%s'"),
                                    svn_path_local_style(unique_name,
                                                         scratch_pool));
        }
      else
        {
          if (delete_when == svn_io_file_del_on_pool_cleanup)
            baton->name = unique_name_apr;

          if (file)
            *file = try_file;
          else
            apr_file_close(try_file);
          if (unique_path) *unique_path = unique_name;

          return SVN_NO_ERROR;
        }
    }

  if (file) *file = NULL;
  if (unique_path) *unique_path = NULL;
  return svn_error_createf(SVN_ERR_IO_UNIQUE_NAMES_EXHAUSTED,
                           NULL,
                           _("Unable to make name for '%s'"),
                           svn_path_local_style(path, scratch_pool));
}

svn_error_t *
svn_io_open_unique_file3(apr_file_t **file,
                         const char **temp_path,
                         const char *dirpath,
                         svn_io_file_del_t delete_when,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  return svn_io_open_uniquely_named(file, temp_path,
                                    dirpath, "tempfile", ".tmp",
                                    delete_when, result_pool, scratch_pool);
}

svn_error_t *
svn_io_create_unique_link(const char **unique_name_p,
                          const char *path,
                          const char *dest,
                          const char *suffix,
                          apr_pool_t *pool)
{
#ifdef HAVE_SYMLINK
  unsigned int i;
  const char *unique_name;
  const char *unique_name_apr;
  const char *dest_apr;
  int rv;

  SVN_ERR(cstring_from_utf8(&dest_apr, dest, pool));
  for (i = 1; i <= 99999; i++)
    {
      apr_status_t apr_err;

      /* Special case the first attempt -- if we can avoid having a
         generated numeric portion at all, that's best.  So first we
         try with just the suffix; then future tries add a number
         before the suffix.  (A do-while loop could avoid the repeated
         conditional, but it's not worth the clarity loss.)

         If the first attempt fails, the first number will be "2".
         This is good, since "1" would misleadingly imply that
         the second attempt was actually the first... and if someone's
         got conflicts on their conflicts, we probably don't want to
         add to their confusion :-). */
      if (i == 1)
        unique_name = apr_psprintf(pool, "%s%s", path, suffix);
      else
        unique_name = apr_psprintf(pool, "%s.%u%s", path, i, suffix);

      /* Hmmm.  Ideally, we would append to a native-encoding buf
         before starting iteration, then convert back to UTF-8 for
         return. But I suppose that would make the appending code
         sensitive to i18n in a way it shouldn't be... Oh well. */
      SVN_ERR(cstring_from_utf8(&unique_name_apr, unique_name, pool));
      do {
        rv = symlink(dest_apr, unique_name_apr);
      } while (rv == -1 && APR_STATUS_IS_EINTR(apr_get_os_error()));

      apr_err = apr_get_os_error();

      if (rv == -1 && APR_STATUS_IS_EEXIST(apr_err))
        continue;
      else if (rv == -1 && apr_err)
        {
          /* On Win32, CreateFile fails with an "Access Denied" error
             code, rather than "File Already Exists", if the colliding
             name belongs to a directory. */
          if (APR_STATUS_IS_EACCES(apr_err))
            {
              apr_finfo_t finfo;
              apr_status_t apr_err_2 = apr_stat(&finfo, unique_name_apr,
                                                APR_FINFO_TYPE, pool);

              if (!apr_err_2
                  && (finfo.filetype == APR_DIR))
                continue;

              /* Else ignore apr_err_2; better to fall through and
                 return the original error. */
            }

          *unique_name_p = NULL;
          return svn_error_wrap_apr(apr_err,
                                    _("Can't create symbolic link '%s'"),
                                    svn_path_local_style(unique_name, pool));
        }
      else
        {
          *unique_name_p = unique_name;
          return SVN_NO_ERROR;
        }
    }

  *unique_name_p = NULL;
  return svn_error_createf(SVN_ERR_IO_UNIQUE_NAMES_EXHAUSTED,
                           NULL,
                           _("Unable to make name for '%s'"),
                           svn_path_local_style(path, pool));
#else
  return svn_error_create(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                          _("Symbolic links are not supported on this "
                            "platform"));
#endif
}

svn_error_t *
svn_io_read_link(svn_string_t **dest,
                 const char *path,
                 apr_pool_t *pool)
{
#ifdef HAVE_READLINK
  svn_string_t dest_apr;
  const char *path_apr;
  char buf[1025];
  int rv;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));
  do {
    rv = readlink(path_apr, buf, sizeof(buf) - 1);
  } while (rv == -1 && APR_STATUS_IS_EINTR(apr_get_os_error()));

  if (rv == -1)
    return svn_error_wrap_apr
      (apr_get_os_error(), _("Can't read contents of link"));

  buf[rv] = '\0';
  dest_apr.data = buf;
  dest_apr.len = rv;

  /* ### Cast needed, one of these interfaces is wrong */
  return svn_utf_string_to_utf8((const svn_string_t **)dest, &dest_apr, pool);
#else
  return svn_error_create(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                          _("Symbolic links are not supported on this "
                            "platform"));
#endif
}


svn_error_t *
svn_io_copy_link(const char *src,
                 const char *dst,
                 apr_pool_t *pool)

{
#ifdef HAVE_READLINK
  svn_string_t *link_dest;
  const char *dst_tmp;

  /* Notice what the link is pointing at... */
  SVN_ERR(svn_io_read_link(&link_dest, src, pool));

  /* Make a tmp-link pointing at the same thing. */
  SVN_ERR(svn_io_create_unique_link(&dst_tmp, dst, link_dest->data,
                                    ".tmp", pool));

  /* Move the tmp-link to link. */
  return svn_io_file_rename(dst_tmp, dst, pool);

#else
  return svn_error_create(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                          _("Symbolic links are not supported on this "
                            "platform"));
#endif
}


svn_error_t *
svn_io_temp_dir(const char **dir,
                apr_pool_t *pool)
{
  apr_status_t apr_err = apr_temp_dir_get(dir, pool);

  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't find a temporary directory"));

  SVN_ERR(cstring_to_utf8(dir, *dir, pool));

  *dir = svn_path_canonicalize(*dir, pool);

  return SVN_NO_ERROR;
}




/*** Creating, copying and appending files. ***/

/* Transfer the contents of FROM_FILE to TO_FILE, using POOL for temporary
 * allocations.
 *
 * NOTE: We don't use apr_copy_file() for this, since it takes filenames
 * as parameters.  Since we want to copy to a temporary file
 * and rename for atomicity (see below), this would require an extra
 * close/open pair, which can be expensive, especially on
 * remote file systems.
 */
static apr_status_t
copy_contents(apr_file_t *from_file,
              apr_file_t *to_file,
              apr_pool_t *pool)
{
  /* Copy bytes till the cows come home. */
  while (1)
    {
      char buf[SVN__STREAM_CHUNK_SIZE];
      apr_size_t bytes_this_time = sizeof(buf);
      apr_status_t read_err;
      apr_status_t write_err;

      /* Read 'em. */
      read_err = apr_file_read(from_file, buf, &bytes_this_time);
      if (read_err && !APR_STATUS_IS_EOF(read_err))
        {
          return read_err;
        }

      /* Write 'em. */
      write_err = apr_file_write_full(to_file, buf, bytes_this_time, NULL);
      if (write_err)
        {
          return write_err;
        }

      if (read_err && APR_STATUS_IS_EOF(read_err))
        {
          /* Return the results of this close: an error, or success. */
          return APR_SUCCESS;
        }
    }
  /* NOTREACHED */
}


svn_error_t *
svn_io_copy_file(const char *src,
                 const char *dst,
                 svn_boolean_t copy_perms,
                 apr_pool_t *pool)
{
  apr_file_t *from_file, *to_file;
  apr_status_t apr_err;
  const char *src_apr, *dst_tmp_apr;
  const char *dst_tmp;
  svn_error_t *err, *err2;

  /* ### NOTE: sometimes src == dst. In this case, because we copy to a
     ###   temporary file, and then rename over the top of the destination,
     ###   the net result is resetting the permissions on src/dst.
     ###
     ### Note: specifically, this can happen during a switch when the desired
     ###   permissions for a file change from one branch to another. See
     ###   switch_tests 17.
     ###
     ### ... yes, we should avoid copying to the same file, and we should
     ###     make the "reset perms" explicit. The switch *happens* to work
     ###     because of this copy-to-temp-then-rename implementation. If it
     ###     weren't for that, the switch would break.
  */
#ifdef CHECK_FOR_SAME_FILE
  if (strcmp(src, dst) == 0)
    return SVN_NO_ERROR;
#endif

  SVN_ERR(cstring_from_utf8(&src_apr, src, pool));

  SVN_ERR(svn_io_file_open(&from_file, src, APR_READ | APR_BINARY,
                         APR_OS_DEFAULT, pool));

  /* For atomicity, we copy to a tmp file and then rename the tmp
     file over the real destination. */

  SVN_ERR(svn_io_open_unique_file3(&to_file, &dst_tmp,
                                   svn_path_dirname(dst, pool),
                                   svn_io_file_del_none, pool, pool));
  SVN_ERR(cstring_from_utf8(&dst_tmp_apr, dst_tmp, pool));

  apr_err = copy_contents(from_file, to_file, pool);

  if (apr_err)
    {
      err = svn_error_wrap_apr
            (apr_err, _("Can't copy '%s' to '%s'"),
             svn_path_local_style(src, pool),
             svn_path_local_style(dst_tmp, pool));
    }
   else
     err = NULL;

  err2 = svn_io_file_close(from_file, pool);
  if (! err)
    err = err2;
  else
    svn_error_clear(err2);
  err2 = svn_io_file_close(to_file, pool);
  if (! err)
    err = err2;
  else
    svn_error_clear(err2);
  if (err)
    {
      apr_err = apr_file_remove(dst_tmp_apr, pool);
      WIN32_RETRY_LOOP(apr_err, apr_file_remove(dst_tmp_apr, pool));
      return err;
    }

  /* If copying perms, set the perms on dst_tmp now, so they will be
     atomically inherited in the upcoming rename.  But note that we
     had to wait until now to set perms, because if they say
     read-only, then we'd have failed filling dst_tmp's contents. */
  if (copy_perms)
    SVN_ERR(svn_io_copy_perms(src, dst_tmp, pool));

  return svn_io_file_rename(dst_tmp, dst, pool);
}


svn_error_t *
svn_io_copy_perms(const char *src,
                  const char *dst,
                  apr_pool_t *pool)
{
  /* ### FIXME: apr_file_copy with perms may fail on Win32.  We need a
     platform-specific implementation to get the permissions right. */

#ifndef WIN32
  {
    apr_file_t *src_file;
    apr_finfo_t finfo;
    const char *dst_apr;
    apr_status_t apr_err;

    SVN_ERR(svn_io_file_open(&src_file, src, APR_READ, APR_OS_DEFAULT, pool));
    SVN_ERR(svn_io_file_info_get(&finfo, APR_FINFO_PROT, src_file, pool));
    SVN_ERR(svn_io_file_close(src_file, pool));

    SVN_ERR(cstring_from_utf8(&dst_apr, dst, pool));
    apr_err = apr_file_perms_set(dst_apr, finfo.protection);

    /* We shouldn't be able to get APR_INCOMPLETE or APR_ENOTIMPL
       here under normal circumstances, because the perms themselves
       came from a call to apr_file_info_get(), and we already know
       this is the non-Win32 case.  But if it does happen, it's not
       an error. */
    if (apr_err != APR_SUCCESS
        && apr_err != APR_INCOMPLETE
        && apr_err != APR_ENOTIMPL)
      {
        return svn_error_wrap_apr(apr_err, _("Can't set permissions on '%s'"),
                                  svn_path_local_style(dst, pool));
      }
  }
#endif /* ! WIN32 */

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_append_file(const char *src, const char *dst, apr_pool_t *pool)
{
  apr_status_t apr_err;
  const char *src_apr, *dst_apr;

  SVN_ERR(cstring_from_utf8(&src_apr, src, pool));
  SVN_ERR(cstring_from_utf8(&dst_apr, dst, pool));

  apr_err = apr_file_append(src_apr, dst_apr, APR_OS_DEFAULT, pool);

  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't append '%s' to '%s'"),
                              svn_path_local_style(src, pool),
                              svn_path_local_style(dst, pool));

  return SVN_NO_ERROR;
}


svn_error_t *svn_io_copy_dir_recursively(const char *src,
                                         const char *dst_parent,
                                         const char *dst_basename,
                                         svn_boolean_t copy_perms,
                                         svn_cancel_func_t cancel_func,
                                         void *cancel_baton,
                                         apr_pool_t *pool)
{
  svn_node_kind_t kind;
  apr_status_t status;
  const char *dst_path;
  apr_dir_t *this_dir;
  apr_finfo_t this_entry;
  apr_int32_t flags = APR_FINFO_TYPE | APR_FINFO_NAME;

  /* Make a subpool for recursion */
  apr_pool_t *subpool = svn_pool_create(pool);

  /* The 'dst_path' is simply dst_parent/dst_basename */
  dst_path = svn_path_join(dst_parent, dst_basename, pool);

  /* Sanity checks:  SRC and DST_PARENT are directories, and
     DST_BASENAME doesn't already exist in DST_PARENT. */
  SVN_ERR(svn_io_check_path(src, &kind, subpool));
  if (kind != svn_node_dir)
    return svn_error_createf(SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                             _("Source '%s' is not a directory"),
                             svn_path_local_style(src, pool));

  SVN_ERR(svn_io_check_path(dst_parent, &kind, subpool));
  if (kind != svn_node_dir)
    return svn_error_createf(SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                             _("Destination '%s' is not a directory"),
                             svn_path_local_style(dst_parent, pool));

  SVN_ERR(svn_io_check_path(dst_path, &kind, subpool));
  if (kind != svn_node_none)
    return svn_error_createf(SVN_ERR_ENTRY_EXISTS, NULL,
                             _("Destination '%s' already exists"),
                             svn_path_local_style(dst_path, pool));

  /* Create the new directory. */
  /* ### TODO: copy permissions (needs apr_file_attrs_get()) */
  SVN_ERR(svn_io_dir_make(dst_path, APR_OS_DEFAULT, pool));

  /* Loop over the dirents in SRC.  ('.' and '..' are auto-excluded) */
  SVN_ERR(svn_io_dir_open(&this_dir, src, subpool));

  for (status = apr_dir_read(&this_entry, flags, this_dir);
       status == APR_SUCCESS;
       status = apr_dir_read(&this_entry, flags, this_dir))
    {
      if ((this_entry.name[0] == '.')
          && ((this_entry.name[1] == '\0')
              || ((this_entry.name[1] == '.')
                  && (this_entry.name[2] == '\0'))))
        {
          continue;
        }
      else
        {
          const char *src_target, *entryname_utf8;

          if (cancel_func)
            SVN_ERR(cancel_func(cancel_baton));

          SVN_ERR(entry_name_to_utf8(&entryname_utf8, this_entry.name,
                                     src, subpool));
          src_target = svn_path_join(src, entryname_utf8, subpool);

          if (this_entry.filetype == APR_REG) /* regular file */
            {
              const char *dst_target = svn_path_join(dst_path, entryname_utf8,
                                                     subpool);
              SVN_ERR(svn_io_copy_file(src_target, dst_target,
                                       copy_perms, subpool));
            }
          else if (this_entry.filetype == APR_LNK) /* symlink */
            {
              const char *dst_target = svn_path_join(dst_path, entryname_utf8,
                                                     subpool);
              SVN_ERR(svn_io_copy_link(src_target, dst_target,
                                       subpool));
            }
          else if (this_entry.filetype == APR_DIR) /* recurse */
            {
              /* Prevent infinite recursion by filtering off our
                 newly created destination path. */
              if (strcmp(src, dst_parent) == 0
                  && strcmp(entryname_utf8, dst_basename) == 0)
                continue;

              SVN_ERR(svn_io_copy_dir_recursively
                      (src_target,
                       dst_path,
                       entryname_utf8,
                       copy_perms,
                       cancel_func,
                       cancel_baton,
                       subpool));
            }
          /* ### support other APR node types someday?? */

        }
    }

  if (! (APR_STATUS_IS_ENOENT(status)))
    return svn_error_wrap_apr(status, _("Can't read directory '%s'"),
                              svn_path_local_style(src, pool));

  status = apr_dir_close(this_dir);
  if (status)
    return svn_error_wrap_apr(status, _("Error closing directory '%s'"),
                              svn_path_local_style(src, pool));

  /* Free any memory used by recursion */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_make_dir_recursively(const char *path, apr_pool_t *pool)
{
  const char *path_apr;
  apr_status_t apr_err;

  if (svn_path_is_empty(path))
    /* Empty path (current dir) is assumed to always exist,
       so we do nothing, per docs. */
    return SVN_NO_ERROR;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  apr_err = apr_dir_make_recursive(path_apr, APR_OS_DEFAULT, pool);
  WIN32_RETRY_LOOP(apr_err, apr_dir_make_recursive(path_apr,
                                                   APR_OS_DEFAULT, pool));

  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't make directory '%s'"),
                              svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
}

svn_error_t *svn_io_file_create(const char *file,
                                const char *contents,
                                apr_pool_t *pool)
{
  apr_file_t *f;
  apr_size_t written;

  SVN_ERR(svn_io_file_open(&f, file,
                           (APR_WRITE | APR_CREATE | APR_EXCL),
                           APR_OS_DEFAULT,
                           pool));
  SVN_ERR(svn_io_file_write_full(f, contents, strlen(contents),
                                 &written, pool));
  return svn_io_file_close(f, pool);
}

svn_error_t *svn_io_dir_file_copy(const char *src_path,
                                  const char *dest_path,
                                  const char *file,
                                  apr_pool_t *pool)
{
  const char *file_dest_path = svn_path_join(dest_path, file, pool);
  const char *file_src_path = svn_path_join(src_path, file, pool);

  return svn_io_copy_file(file_src_path, file_dest_path, TRUE, pool);
}


/*** Modtime checking. ***/

svn_error_t *
svn_io_file_affected_time(apr_time_t *apr_time,
                          const char *path,
                          apr_pool_t *pool)
{
  apr_finfo_t finfo;

  SVN_ERR(svn_io_stat(&finfo, path, APR_FINFO_MIN | APR_FINFO_LINK, pool));

  *apr_time = finfo.mtime;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_set_file_affected_time(apr_time_t apr_time,
                              const char *path,
                              apr_pool_t *pool)
{
  apr_status_t status;
  const char *native_path;

  SVN_ERR(cstring_from_utf8(&native_path, path, pool));
  status = apr_file_mtime_set(native_path, apr_time, pool);

  if (status)
    return svn_error_wrap_apr
      (status, _("Can't set access time of '%s'"),
       svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
}


void
svn_io_sleep_for_timestamps(const char *path, apr_pool_t *pool)
{
  apr_time_t now, then;
  svn_error_t *err;
  char *sleep_env_var;

  sleep_env_var = getenv(SVN_SLEEP_ENV_VAR);

  if (sleep_env_var && apr_strnatcasecmp(sleep_env_var, "yes") == 0)
    return; /* Allow skipping for testing */

  now = apr_time_now();

  /* Calculate 0.02 seconds after the next second wallclock tick. */
  then = apr_time_make(apr_time_sec(now) + 1, APR_USEC_PER_SEC / 50);

  /* Worst case is waiting one second, so we can use that time to determine
     if we can sleep shorter than that */
  if (path)
    {
      apr_finfo_t finfo;

      err = svn_io_stat(&finfo, path, APR_FINFO_MTIME | APR_FINFO_LINK, pool);

      if (err)
        {
          svn_error_clear(err); /* Fall back on original behavior */
        }
      else if (finfo.mtime % APR_USEC_PER_SEC)
        {
          /* Very simplistic but safe approach:
              If the filesystem has < sec mtime we can be reasonably sure
              that the filesystem has <= millisecond precision.

             ## Perhaps find a better algorithm here. This will fail once
                in every 1000 cases on a millisecond precision filesystem.

                But better to fail once in every thousand cases than every
                time, like we did before.
                (All tested filesystems I know have at least microsecond precision.)

             Note for further research on algorithm:
               FAT32 has < 1 sec precision on ctime, but 2 sec on mtime */

          /* Sleep for at least 1 millisecond.
             (t < 1000 will be round to 0 in apr) */
          apr_sleep(1000);

          return;
        }

      now = apr_time_now(); /* Extract the time used for the path stat */

      if (now >= then)
        return; /* Passing negative values may suspend indefinately (Windows) */
    }

  apr_sleep(then - now);
}


svn_error_t *
svn_io_filesizes_different_p(svn_boolean_t *different_p,
                             const char *file1,
                             const char *file2,
                             apr_pool_t *pool)
{
  apr_finfo_t finfo1;
  apr_finfo_t finfo2;
  apr_status_t status;
  const char *file1_apr, *file2_apr;

  /* Not using svn_io_stat() because don't want to generate
     svn_error_t objects for non-error conditions. */

  SVN_ERR(cstring_from_utf8(&file1_apr, file1, pool));
  SVN_ERR(cstring_from_utf8(&file2_apr, file2, pool));

  /* Stat both files */
  status = apr_stat(&finfo1, file1_apr, APR_FINFO_MIN, pool);
  if (status)
    {
      /* If we got an error stat'ing a file, it could be because the
         file was removed... or who knows.  Whatever the case, we
         don't know if the filesizes are definitely different, so
         assume that they're not. */
      *different_p = FALSE;
      return SVN_NO_ERROR;
    }

  status = apr_stat(&finfo2, file2_apr, APR_FINFO_MIN, pool);
  if (status)
    {
      /* See previous comment. */
      *different_p = FALSE;
      return SVN_NO_ERROR;
    }

  /* Examine file sizes */
  if (finfo1.size == finfo2.size)
    *different_p = FALSE;
  else
    *different_p = TRUE;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_file_checksum2(svn_checksum_t **checksum,
                      const char *file,
                      svn_checksum_kind_t kind,
                      apr_pool_t *pool)
{
  svn_stream_t *file_stream;
  svn_stream_t *checksum_stream;
  apr_file_t* f;

  SVN_ERR(svn_io_file_open(&f, file, APR_READ, APR_OS_DEFAULT, pool));
  file_stream = svn_stream_from_aprfile2(f, FALSE, pool);
  checksum_stream = svn_stream_checksummed2(file_stream, checksum, NULL, kind,
                                            TRUE, pool);

  /* Because the checksummed stream will force the reading (and
     checksumming) of all the file's bytes, we can just close the stream
     and let its magic work. */
  return svn_stream_close(checksum_stream);
}


svn_error_t *
svn_io_file_checksum(unsigned char digest[],
                     const char *file,
                     apr_pool_t *pool)
{
  svn_checksum_t *checksum;

  SVN_ERR(svn_io_file_checksum2(&checksum, file, svn_checksum_md5, pool));
  memcpy(digest, checksum->digest, APR_MD5_DIGESTSIZE);

  return SVN_NO_ERROR;
}



/*** Permissions and modes. ***/

#ifndef WIN32
/* Given the file specified by PATH, attempt to create an
   identical version of it owned by the current user.  This is done by
   moving it to a temporary location, copying the file back to its old
   path, then deleting the temporarily moved version.  All temporary
   allocations are done in POOL. */
static svn_error_t *
reown_file(const char *path,
           apr_pool_t *pool)
{
  const char *unique_name;

  SVN_ERR(svn_io_open_unique_file3(NULL, &unique_name,
                                   svn_path_dirname(path, pool),
                                   svn_io_file_del_none, pool, pool));
  SVN_ERR(svn_io_file_rename(path, unique_name, pool));
  SVN_ERR(svn_io_copy_file(unique_name, path, TRUE, pool));
  return svn_io_remove_file(unique_name, pool);
}

/* Determine what the read-write PERMS for PATH should be by ORing
   together the permissions of PATH and the permissions of a temporary
   file that we create.  Unfortunately, this is the only way to
   determine which combination of write bits (User/Group/World) should
   be set to restore a file from read-only to read-write.  Make
   temporary allocations in POOL.  */
static svn_error_t *
get_default_file_perms(const char *path, apr_fileperms_t *perms,
                       apr_pool_t *pool)
{
  apr_status_t status;
  apr_finfo_t tmp_finfo, finfo;
  apr_file_t *fd;
  const char *tmp_path;
  const char *apr_path;

  /* Get the perms for a newly created file to find out what write
     bits should be set.

     NOTE: normally del_on_close can be problematic because APR might
       delete the file if we spawned any child processes. In this case,
       the lifetime of this file handle is about 3 lines of code, so
       we can safely use del_on_close here.

     NOTE: not so fast, shorty. if some other thread forks off a child,
       then the APR cleanups run, and the file will disappear. sigh.
  */
  SVN_ERR(svn_io_open_unique_file3(&fd, &tmp_path,
                                   svn_path_dirname(path, pool),
                                   svn_io_file_del_on_pool_cleanup,
                                   pool, pool));
  status = apr_stat(&tmp_finfo, tmp_path, APR_FINFO_PROT, pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't get default file perms "
                                        "for file at '%s' (file stat error)"),
                              path);
  apr_file_close(fd);

  /* Get the perms for the original file so we'll have any other bits
   * that were already set (like the execute bits, for example). */
  SVN_ERR(cstring_from_utf8(&apr_path, path, pool));
  status = apr_file_open(&fd, apr_path, APR_READ | APR_BINARY,
                         APR_OS_DEFAULT, pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't open file at '%s'"), path);

  status = apr_stat(&finfo, apr_path, APR_FINFO_PROT, pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't get file perms for file at "
                                        "'%s' (file stat error)"), path);
  apr_file_close(fd);

  /* Glom the perms together. */
  *perms = tmp_finfo.protection | finfo.protection;
  return SVN_NO_ERROR;
}

/* This is a helper function for the svn_io_set_file_read* functions
   that attempts to honor the users umask when dealing with
   permission changes.  It is a no-op when invoked on a symlink. */
static svn_error_t *
io_set_file_perms(const char *path,
                  svn_boolean_t change_readwrite,
                  svn_boolean_t enable_write,
                  svn_boolean_t change_executable,
                  svn_boolean_t executable,
                  svn_boolean_t ignore_enoent,
                  apr_pool_t *pool)
{
  apr_status_t status;
  const char *path_apr;
  apr_finfo_t finfo;
  apr_fileperms_t perms_to_set;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  /* Try to change only a minimal amount of the perms first
     by getting the current perms and adding bits
     only on where read perms are granted.  If this fails
     fall through to just setting file attributes. */
  status = apr_stat(&finfo, path_apr, APR_FINFO_PROT | APR_FINFO_LINK, pool);
  if (status)
    {
      if (ignore_enoent && APR_STATUS_IS_ENOENT(status))
        return SVN_NO_ERROR;
      else if (status != APR_ENOTIMPL)
        return svn_error_wrap_apr(status,
                                  _("Can't change perms of file '%s'"),
                                  svn_path_local_style(path, pool));
      return SVN_NO_ERROR;
    }

  if (finfo.filetype == APR_LNK)
    return SVN_NO_ERROR;

  perms_to_set = finfo.protection;
  if (change_readwrite)
    {
      if (enable_write) /* Make read-write. */
        SVN_ERR(get_default_file_perms(path, &perms_to_set, pool));
      else
        {
          if (finfo.protection & APR_UREAD)
            perms_to_set &= ~APR_UWRITE;
          if (finfo.protection & APR_GREAD)
            perms_to_set &= ~APR_GWRITE;
          if (finfo.protection & APR_WREAD)
            perms_to_set &= ~APR_WWRITE;
        }
    }

  if (change_executable)
    {
      if (executable)
        {
          if (finfo.protection & APR_UREAD)
            perms_to_set |= APR_UEXECUTE;
          if (finfo.protection & APR_GREAD)
            perms_to_set |= APR_GEXECUTE;
          if (finfo.protection & APR_WREAD)
            perms_to_set |= APR_WEXECUTE;
        }
      else
        {
          if (finfo.protection & APR_UREAD)
            perms_to_set &= ~APR_UEXECUTE;
          if (finfo.protection & APR_GREAD)
            perms_to_set &= ~APR_GEXECUTE;
          if (finfo.protection & APR_WREAD)
            perms_to_set &= ~APR_WEXECUTE;
        }
    }

  /* If we aren't changing anything then just return, this saves
     some system calls and helps with shared working copies */
  if (perms_to_set == finfo.protection)
    return SVN_NO_ERROR;

  status = apr_file_perms_set(path_apr, perms_to_set);
  if (!status)
    return SVN_NO_ERROR;

  if (APR_STATUS_IS_EPERM(status))
    {
      /* We don't have permissions to change the
         permissions!  Try a move, copy, and delete
         workaround to see if we can get the file owned by
         us.  If these succeed, try the permissions set
         again.

         Note that we only attempt this in the
         stat-available path.  This assumes that the
         move-copy workaround will only be helpful on
         platforms that implement apr_stat. */
      SVN_ERR(reown_file(path, pool));
      status = apr_file_perms_set(path_apr, perms_to_set);
    }

  if (!status)
    return SVN_NO_ERROR;

  if (ignore_enoent && APR_STATUS_IS_ENOENT(status))
    return SVN_NO_ERROR;
  else if (status == APR_ENOTIMPL)
    {
      /* At least try to set the attributes. */
      apr_fileattrs_t attrs = 0;
      apr_fileattrs_t attrs_values = 0;

      if (change_readwrite)
        {
          attrs = APR_FILE_ATTR_READONLY;
          if (!enable_write)
            attrs_values = APR_FILE_ATTR_READONLY;
        }
      if (change_executable)
        {
          attrs = APR_FILE_ATTR_EXECUTABLE;
          if (executable)
            attrs_values = APR_FILE_ATTR_EXECUTABLE;
        }
      status = apr_file_attrs_set(path_apr, attrs, attrs_values, pool);
    }

  return svn_error_wrap_apr(status,
                            _("Can't change perms of file '%s'"),
                            svn_path_local_style(path, pool));
}
#endif /* !WIN32 */


svn_error_t *
svn_io_set_file_read_write_carefully(const char *path,
                                     svn_boolean_t enable_write,
                                     svn_boolean_t ignore_enoent,
                                     apr_pool_t *pool)
{
  if (enable_write)
    return svn_io_set_file_read_write(path, ignore_enoent, pool);
  return svn_io_set_file_read_only(path, ignore_enoent, pool);
}

svn_error_t *
svn_io_set_file_read_only(const char *path,
                          svn_boolean_t ignore_enoent,
                          apr_pool_t *pool)
{
  /* On Windows, just set the file attributes -- on unix call
     our internal function which attempts to honor the umask. */
#ifndef WIN32
  return io_set_file_perms(path, TRUE, FALSE, FALSE, FALSE,
                           ignore_enoent, pool);
#else
  apr_status_t status;
  const char *path_apr;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  status = apr_file_attrs_set(path_apr,
                              APR_FILE_ATTR_READONLY,
                              APR_FILE_ATTR_READONLY,
                              pool);

  if (status && status != APR_ENOTIMPL)
    if (!ignore_enoent || !APR_STATUS_IS_ENOENT(status))
      return svn_error_wrap_apr(status,
                                _("Can't set file '%s' read-only"),
                                svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_io_set_file_read_write(const char *path,
                           svn_boolean_t ignore_enoent,
                           apr_pool_t *pool)
{
  /* On Windows, just set the file attributes -- on unix call
     our internal function which attempts to honor the umask. */
#ifndef WIN32
  return io_set_file_perms(path, TRUE, TRUE, FALSE, FALSE,
                           ignore_enoent, pool);
#else
  apr_status_t status;
  const char *path_apr;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  status = apr_file_attrs_set(path_apr,
                              0,
                              APR_FILE_ATTR_READONLY,
                              pool);

  if (status && status != APR_ENOTIMPL)
    if (!ignore_enoent || !APR_STATUS_IS_ENOENT(status))
      return svn_error_wrap_apr(status,
                                _("Can't set file '%s' read-write"),
                                svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
#endif
}

svn_error_t *
svn_io_set_file_executable(const char *path,
                           svn_boolean_t executable,
                           svn_boolean_t ignore_enoent,
                           apr_pool_t *pool)
{
  /* On Windows, just exit -- on unix call our internal function
  which attempts to honor the umask. */
#ifndef WIN32
  return io_set_file_perms(path, FALSE, FALSE, TRUE, executable,
                           ignore_enoent, pool);
#else
  return SVN_NO_ERROR;
#endif
}


svn_error_t *
svn_io_is_file_executable(svn_boolean_t *executable,
                          const char *path,
                          apr_pool_t *pool)
{
#if defined(APR_HAS_USER) && !defined(WIN32)
  apr_finfo_t file_info;
  apr_status_t apr_err;
  apr_uid_t uid;
  apr_gid_t gid;

  *executable = FALSE;

  /* Get file and user info. */
  SVN_ERR(svn_io_stat(&file_info, path,
                      (APR_FINFO_PROT | APR_FINFO_OWNER),
                      pool));
  apr_err = apr_uid_current(&uid, &gid, pool);

  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Error getting UID of process"));

  /* Check executable bit for current user. */
  if (apr_uid_compare(uid, file_info.user) == APR_SUCCESS)
    *executable = (file_info.protection & APR_UEXECUTE);

  else if (apr_gid_compare(gid, file_info.group) == APR_SUCCESS)
    *executable = (file_info.protection & APR_GEXECUTE);

  else
    *executable = (file_info.protection & APR_WEXECUTE);

#else  /* defined(WIN32) || !defined(APR_HAS_USER) */
  *executable = FALSE;
#endif

  return SVN_NO_ERROR;
}


/*** File locking. ***/
/* Clear all outstanding locks on ARG, an open apr_file_t *. */
static apr_status_t
svn_io__file_clear_and_close(void *arg)
{
  apr_status_t apr_err;
  apr_file_t *f = arg;

  /* Remove locks. */
  apr_err = apr_file_unlock(f);
  if (apr_err)
    return apr_err;

  /* Close the file. */
  apr_err = apr_file_close(f);
  if (apr_err)
    return apr_err;

  return 0;
}


svn_error_t *svn_io_file_lock(const char *lock_file,
                              svn_boolean_t exclusive,
                              apr_pool_t *pool)
{
  return svn_io_file_lock2(lock_file, exclusive, FALSE, pool);
}

svn_error_t *svn_io_file_lock2(const char *lock_file,
                               svn_boolean_t exclusive,
                               svn_boolean_t nonblocking,
                               apr_pool_t *pool)
{
  int locktype = APR_FLOCK_SHARED;
  apr_file_t *lockfile_handle;
  apr_int32_t flags;
  apr_status_t apr_err;

  if (exclusive == TRUE)
    locktype = APR_FLOCK_EXCLUSIVE;

  flags = APR_READ;
  if (locktype == APR_FLOCK_EXCLUSIVE)
    flags |= APR_WRITE;

  if (nonblocking == TRUE)
    locktype |= APR_FLOCK_NONBLOCK;

  SVN_ERR(svn_io_file_open(&lockfile_handle, lock_file, flags,
                           APR_OS_DEFAULT,
                           pool));

  /* Get lock on the filehandle. */
  apr_err = apr_file_lock(lockfile_handle, locktype);
  if (apr_err)
    {
      switch (locktype & APR_FLOCK_TYPEMASK)
        {
        case APR_FLOCK_SHARED:
          return svn_error_wrap_apr
            (apr_err, _("Can't get shared lock on file '%s'"),
             svn_path_local_style(lock_file, pool));
        case APR_FLOCK_EXCLUSIVE:
          return svn_error_wrap_apr
            (apr_err, _("Can't get exclusive lock on file '%s'"),
             svn_path_local_style(lock_file, pool));
        default:
          SVN_ERR_MALFUNCTION();
        }
    }

  apr_pool_cleanup_register(pool, lockfile_handle,
                            svn_io__file_clear_and_close,
                            apr_pool_cleanup_null);

  return SVN_NO_ERROR;
}



/* Data consistency/coherency operations. */

static svn_error_t *
do_io_file_wrapper_cleanup(apr_file_t *file, apr_status_t status,
                           const char *msg, const char *msg_no_name,
                           apr_pool_t *pool);

svn_error_t *svn_io_file_flush_to_disk(apr_file_t *file,
                                       apr_pool_t *pool)
{
  apr_os_file_t filehand;

  /* First make sure that any user-space buffered data is flushed. */
  SVN_ERR(do_io_file_wrapper_cleanup(file, apr_file_flush(file),
                                     N_("Can't flush file '%s'"),
                                     N_("Can't flush stream"),
                                     pool));

  apr_os_file_get(&filehand, file);

  /* Call the operating system specific function to actually force the
     data to disk. */
  {
#ifdef WIN32

    if (! FlushFileBuffers(filehand))
        return svn_error_wrap_apr
          (apr_get_os_error(), _("Can't flush file to disk"));

#else
      int rv;

      do {
        rv = fsync(filehand);
      } while (rv == -1 && APR_STATUS_IS_EINTR(apr_get_os_error()));

      /* If the file is in a memory filesystem, fsync() may return
         EINVAL.  Presumably the user knows the risks, and we can just
         ignore the error. */
      if (rv == -1 && APR_STATUS_IS_EINVAL(apr_get_os_error()))
        return SVN_NO_ERROR;

      if (rv == -1)
        return svn_error_wrap_apr
          (apr_get_os_error(), _("Can't flush file to disk"));

#endif
  }
  return SVN_NO_ERROR;
}



/* TODO write test for these two functions, then refactor. */

svn_error_t *
svn_stringbuf_from_file2(svn_stringbuf_t **result,
                         const char *filename,
                         apr_pool_t *pool)
{
  apr_file_t *f;

  if (filename[0] == '-' && filename[1] == '\0')
    {
      apr_status_t apr_err;
      if ((apr_err = apr_file_open_stdin(&f, pool)))
        return svn_error_wrap_apr(apr_err, _("Can't open stdin"));
    }
  else
    {
      SVN_ERR(svn_io_file_open(&f, filename, APR_READ, APR_OS_DEFAULT, pool));
    }

  /* ### ugh. we should stat() the file, get its length, and read that
     ### much data into memory. the _from_aprfile() function uses a
     ### realloc-style that chews up memory needlessly. */

  SVN_ERR(svn_stringbuf_from_aprfile(result, f, pool));
  return svn_io_file_close(f, pool);
}


svn_error_t *
svn_stringbuf_from_file(svn_stringbuf_t **result,
                        const char *filename,
                        apr_pool_t *pool)
{
  if (filename[0] == '-' && filename[1] == '\0')
    return svn_error_create
        (SVN_ERR_UNSUPPORTED_FEATURE, NULL,
         _("Reading from stdin is disallowed"));
  return svn_stringbuf_from_file2(result, filename, pool);
}


/* Get the name of FILE, or NULL if FILE is an unnamed stream. */
static svn_error_t *
file_name_get(const char **fname_utf8, apr_file_t *file, apr_pool_t *pool)
{
  apr_status_t apr_err;
  const char *fname;

  apr_err = apr_file_name_get(&fname, file);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't get file name"));

  if (fname)
    SVN_ERR(svn_path_cstring_to_utf8(fname_utf8, fname, pool));
  else
    *fname_utf8 = NULL;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_stringbuf_from_aprfile(svn_stringbuf_t **result,
                           apr_file_t *file,
                           apr_pool_t *pool)
{
  apr_size_t len;
  svn_error_t *err;
  svn_stringbuf_t *res = svn_stringbuf_create_ensure(SVN__STREAM_CHUNK_SIZE,
                                                     pool);
  char *buf = apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);

  /* XXX: We should check the incoming data for being of type binary. */

  /* apr_file_read will not return data and eof in the same call. So this loop
   * is safe from missing read data.  */
  len = SVN__STREAM_CHUNK_SIZE;
  err = svn_io_file_read(file, buf, &len, pool);
  while (! err)
    {
      svn_stringbuf_appendbytes(res, buf, len);
      len = SVN__STREAM_CHUNK_SIZE;
      err = svn_io_file_read(file, buf, &len, pool);
    }

  /* Having read all the data we *expect* EOF */
  if (err && !APR_STATUS_IS_EOF(err->apr_err))
    return err;
  svn_error_clear(err);

  *result = res;
  return SVN_NO_ERROR;
}



/* Deletion. */

svn_error_t *
svn_io_remove_file(const char *path, apr_pool_t *pool)
{
  apr_status_t apr_err;
  const char *path_apr;

#ifdef WIN32
  /* Set the file writable but only on Windows, because Windows
     will not allow us to remove files that are read-only. */
  SVN_ERR(svn_io_set_file_read_write(path, TRUE, pool));
#endif /* WIN32 */

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  apr_err = apr_file_remove(path_apr, pool);
#ifdef WIN32
  if (apr_err)
    {
      /* Check to make sure we aren't trying to delete a directory */
      if (APR_TO_OS_ERROR(apr_err) == ERROR_ACCESS_DENIED)
        {
          apr_finfo_t finfo;

          if (apr_stat(&finfo, path_apr, APR_FINFO_TYPE, pool) == APR_SUCCESS 
              && finfo.filetype == APR_REG)
            {
              WIN32_RETRY_LOOP(apr_err, apr_file_remove(path_apr, pool));
            }		  
        }

      /* Just return the delete error */
    }
#endif
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't remove file '%s'"),
                              svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_remove_dir(const char *path, apr_pool_t *pool)
{
  return svn_io_remove_dir2(path, FALSE, NULL, NULL, pool);
}

/*
 Mac OS X has a bug where if you're readding the contents of a
 directory via readdir in a loop, and you remove one of the entries in
 the directory and the directory has 338 or more files in it you will
 skip over some of the entries in the directory.  Needless to say,
 this causes problems if you are using this kind of loop inside a
 function that is recursively deleting a directory, because when you
 get around to removing the directory it will still have something in
 it.

 Similar problem has been observed on FreeBSD.

 See http://subversion.tigris.org/issues/show_bug.cgi?id=1896 for more
 discussion and an initial solution.

 To work around the problem, we do a rewinddir after we delete all files
 and see if there's anything left. We repeat the steps untill there's
 nothing left to delete.

 This workaround causes issues on Windows where delete's are asynchronous,
 however, so we never rewind if we're on Windows (the delete says it is
 complete, we rewind, we see the same file and try to delete it again,
 we fail.
*/

/* Neither windows nor unix allows us to delete a non-empty
   directory.

   This is a function to perform the equivalent of 'rm -rf'. */
svn_error_t *
svn_io_remove_dir2(const char *path, svn_boolean_t ignore_enoent,
                   svn_cancel_func_t cancel_func, void *cancel_baton,
                   apr_pool_t *pool)
{
  apr_status_t status;
  apr_dir_t *this_dir;
  apr_finfo_t this_entry;
  apr_pool_t *subpool;
  apr_int32_t flags = APR_FINFO_TYPE | APR_FINFO_NAME;
  const char *path_apr;
  int need_rewind;

  /* Check for pending cancellation request.
     If we need to bail out, do so early. */

  if (cancel_func)
    SVN_ERR((*cancel_func)(cancel_baton));

  /* Convert path to native here and call apr_dir_open directly,
     instead of just using svn_io_dir_open, because we're going to
     need path_apr later anyway when we remove the dir itself. */

  if (path[0] == '\0')
    /* APR doesn't like "" directories; use "." instead. */
    SVN_ERR(cstring_from_utf8(&path_apr, ".", pool));
  else
    SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  status = apr_dir_open(&this_dir, path_apr, pool);
  if (status)
    {
      /* if the directory doesn't exist, our mission is accomplished */
      if (ignore_enoent && APR_STATUS_IS_ENOENT(status))
        return SVN_NO_ERROR;
      else
        return svn_error_wrap_apr(status,
                                  _("Can't open directory '%s'"),
                                  svn_path_local_style(path, pool));
    }

  subpool = svn_pool_create(pool);

  do
    {
      need_rewind = FALSE;

      for (status = apr_dir_read(&this_entry, flags, this_dir);
           status == APR_SUCCESS;
           status = apr_dir_read(&this_entry, flags, this_dir))
        {
          svn_pool_clear(subpool);
          if ((this_entry.filetype == APR_DIR)
              && ((this_entry.name[0] == '.')
                  && ((this_entry.name[1] == '\0')
                      || ((this_entry.name[1] == '.')
                          && (this_entry.name[2] == '\0')))))
            {
              continue;
            }
          else  /* something other than "." or "..", so proceed */
            {
              const char *fullpath, *entry_utf8;

#ifndef WIN32
              need_rewind = TRUE;
#endif

              SVN_ERR(entry_name_to_utf8(&entry_utf8, this_entry.name,
                                         path_apr, subpool));

              fullpath = svn_path_join(path, entry_utf8, subpool);

              if (this_entry.filetype == APR_DIR)
                {
                  /* Don't check for cancellation, the callee
                     will immediately do so */
                  SVN_ERR(svn_io_remove_dir2(fullpath, FALSE,
                                             cancel_func, cancel_baton,
                                             subpool));
                }
              else
                {
                  svn_error_t *err;

                  if (cancel_func)
                    SVN_ERR((*cancel_func)(cancel_baton));

                  err = svn_io_remove_file(fullpath, subpool);
                  if (err)
                    return svn_error_createf
                      (err->apr_err, err, _("Can't remove '%s'"),
                       svn_path_local_style(fullpath, subpool));
                }
            }
        }

      if (need_rewind)
        {
          status = apr_dir_rewind(this_dir);
          if (status)
            return svn_error_wrap_apr(status, _("Can't rewind directory '%s'"),
                                      svn_path_local_style (path, pool));
        }
    }
  while (need_rewind);

  svn_pool_destroy(subpool);

  if (!APR_STATUS_IS_ENOENT(status))
    return svn_error_wrap_apr(status, _("Can't read directory '%s'"),
                              svn_path_local_style(path, pool));

  status = apr_dir_close(this_dir);
  if (status)
    return svn_error_wrap_apr(status, _("Error closing directory '%s'"),
                              svn_path_local_style(path, pool));

  status = apr_dir_remove(path_apr, pool);
  WIN32_RETRY_LOOP(status, apr_dir_remove(path_apr, pool));
  if (status)
    return svn_error_wrap_apr(status, _("Can't remove '%s'"),
                              svn_path_local_style(path, pool));

  return APR_SUCCESS;
}

svn_error_t *
svn_io_get_dir_filenames(apr_hash_t **dirents,
                         const char *path,
                         apr_pool_t *pool)
{
  apr_status_t status;
  apr_dir_t *this_dir;
  apr_finfo_t this_entry;
  apr_int32_t flags = APR_FINFO_NAME;

  *dirents = apr_hash_make(pool);

  SVN_ERR(svn_io_dir_open(&this_dir, path, pool));

  for (status = apr_dir_read(&this_entry, flags, this_dir);
       status == APR_SUCCESS;
       status = apr_dir_read(&this_entry, flags, this_dir))
    {
      if ((this_entry.name[0] == '.')
          && ((this_entry.name[1] == '\0')
              || ((this_entry.name[1] == '.')
                  && (this_entry.name[2] == '\0'))))
        {
          continue;
        }
      else
        {
          const char *name;
          SVN_ERR(entry_name_to_utf8(&name, this_entry.name, path, pool));
          apr_hash_set(*dirents, name, APR_HASH_KEY_STRING, name);
        }
    }

  if (! (APR_STATUS_IS_ENOENT(status)))
    return svn_error_wrap_apr(status, _("Can't read directory '%s'"),
                              svn_path_local_style(path, pool));

  status = apr_dir_close(this_dir);
  if (status)
    return svn_error_wrap_apr(status, _("Error closing directory '%s'"),
                              svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_io_get_dirents2(apr_hash_t **dirents,
                    const char *path,
                    apr_pool_t *pool)
{
  apr_status_t status;
  apr_dir_t *this_dir;
  apr_finfo_t this_entry;
  apr_int32_t flags = APR_FINFO_TYPE | APR_FINFO_NAME;

  *dirents = apr_hash_make(pool);

  SVN_ERR(svn_io_dir_open(&this_dir, path, pool));

  for (status = apr_dir_read(&this_entry, flags, this_dir);
       status == APR_SUCCESS;
       status = apr_dir_read(&this_entry, flags, this_dir))
    {
      if ((this_entry.name[0] == '.')
          && ((this_entry.name[1] == '\0')
              || ((this_entry.name[1] == '.')
                  && (this_entry.name[2] == '\0'))))
        {
          continue;
        }
      else
        {
          const char *name;
          svn_io_dirent_t *dirent = apr_palloc(pool, sizeof(*dirent));

          SVN_ERR(entry_name_to_utf8(&name, this_entry.name, path, pool));

          map_apr_finfo_to_node_kind(&(dirent->kind),
                                     &(dirent->special),
                                     &this_entry);

          apr_hash_set(*dirents, name, APR_HASH_KEY_STRING, dirent);
        }
    }

  if (! (APR_STATUS_IS_ENOENT(status)))
    return svn_error_wrap_apr(status, _("Can't read directory '%s'"),
                              svn_path_local_style(path, pool));

  status = apr_dir_close(this_dir);
  if (status)
    return svn_error_wrap_apr(status, _("Error closing directory '%s'"),
                              svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_io_get_dirents(apr_hash_t **dirents,
                   const char *path,
                   apr_pool_t *pool)
{
  /* Note that in C, padding is not allowed at the beginning of structs,
     so this is actually portable, since the kind field of svn_io_dirent_t
     is first in that struct. */
  return svn_io_get_dirents2(dirents, path, pool);
}

/* Pool userdata key for the error file passed to svn_io_start_cmd(). */
#define ERRFILE_KEY "svn-io-start-cmd-errfile"

/* Handle an error from the child process (before command execution) by
   printing DESC and the error string corresponding to STATUS to stderr. */
static void
handle_child_process_error(apr_pool_t *pool, apr_status_t status,
                           const char *desc)
{
  char errbuf[256];
  apr_file_t *errfile;
  void *p;

  /* We can't do anything if we get an error here, so just return. */
  if (apr_pool_userdata_get(&p, ERRFILE_KEY, pool))
    return;
  errfile = p;

  if (errfile)
    /* What we get from APR is in native encoding. */
    apr_file_printf(errfile, "%s: %s",
                    desc, apr_strerror(status, errbuf,
                                       sizeof(errbuf)));
}


svn_error_t *
svn_io_start_cmd(apr_proc_t *cmd_proc,
                 const char *path,
                 const char *cmd,
                 const char *const *args,
                 svn_boolean_t inherit,
                 apr_file_t *infile,
                 apr_file_t *outfile,
                 apr_file_t *errfile,
                 apr_pool_t *pool)
{
  apr_status_t apr_err;
  apr_procattr_t *cmdproc_attr;
  int num_args;
  const char **args_native;
  const char *cmd_apr;

  /* Create the process attributes. */
  apr_err = apr_procattr_create(&cmdproc_attr, pool);
  if (apr_err)
    return svn_error_wrap_apr
      (apr_err, _("Can't create process '%s' attributes"), cmd);

  /* Make sure we invoke cmd directly, not through a shell. */
  apr_err = apr_procattr_cmdtype_set(cmdproc_attr,
                                     inherit?APR_PROGRAM_PATH:APR_PROGRAM);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't set process '%s' cmdtype"),
                              cmd);

  /* Set the process's working directory. */
  if (path)
    {
      const char *path_apr;

      SVN_ERR(cstring_from_utf8(&path_apr, path, pool));
      apr_err = apr_procattr_dir_set(cmdproc_attr, path_apr);
      if (apr_err)
        return svn_error_wrap_apr
          (apr_err, _("Can't set process '%s' directory"), cmd);
    }

  /* Use requested inputs and outputs.

     ### Unfortunately each of these apr functions creates a pipe and then
     overwrites the pipe file descriptor with the descriptor we pass
     in. The pipes can then never be closed. This is an APR bug. */
  if (infile)
    {
      apr_err = apr_procattr_child_in_set(cmdproc_attr, infile, NULL);
      if (apr_err)
        return svn_error_wrap_apr
          (apr_err, _("Can't set process '%s' child input"), cmd);
    }
  if (outfile)
    {
      apr_err = apr_procattr_child_out_set(cmdproc_attr, outfile, NULL);
      if (apr_err)
        return svn_error_wrap_apr
          (apr_err, _("Can't set process '%s' child outfile"), cmd);
    }
  if (errfile)
    {
      apr_err = apr_procattr_child_err_set(cmdproc_attr, errfile, NULL);
      if (apr_err)
        return svn_error_wrap_apr
          (apr_err, _("Can't set process '%s' child errfile"), cmd);
    }

  /* Have the child print any problems executing its program to errfile. */
  apr_err = apr_pool_userdata_set(errfile, ERRFILE_KEY, NULL, pool);
  if (apr_err)
    return svn_error_wrap_apr
      (apr_err, _("Can't set process '%s' child errfile for error handler"),
       cmd);
  apr_err = apr_procattr_child_errfn_set(cmdproc_attr,
                                         handle_child_process_error);
  if (apr_err)
    return svn_error_wrap_apr
      (apr_err, _("Can't set process '%s' error handler"), cmd);

  /* Convert cmd and args from UTF-8 */
  SVN_ERR(cstring_from_utf8(&cmd_apr, cmd, pool));
  for (num_args = 0; args[num_args]; num_args++)
    ;
  args_native = apr_palloc(pool, (num_args + 1) * sizeof(char *));
  args_native[num_args] = NULL;
  while (num_args--)
    {
      /* ### Well, it turns out that on APR on Windows expects all
             program args to be in UTF-8. Callers of svn_io_run_cmd
             should be aware of that. */
      SVN_ERR(cstring_from_utf8(&args_native[num_args],
                                args[num_args], pool));
    }


  /* Start the cmd command. */
  apr_err = apr_proc_create(cmd_proc, cmd_apr, args_native, NULL,
                            cmdproc_attr, pool);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't start process '%s'"), cmd);

  return SVN_NO_ERROR;
}

#undef ERRFILE_KEY

svn_error_t *
svn_io_wait_for_cmd(apr_proc_t *cmd_proc,
                    const char *cmd,
                    int *exitcode,
                    apr_exit_why_e *exitwhy,
                    apr_pool_t *pool)
{
  apr_status_t apr_err;
  apr_exit_why_e exitwhy_val;
  int exitcode_val;

  /* The Win32 apr_proc_wait doesn't set this... */
  exitwhy_val = APR_PROC_EXIT;

  /* Wait for the cmd command to finish. */
  apr_err = apr_proc_wait(cmd_proc, &exitcode_val, &exitwhy_val, APR_WAIT);
  if (!APR_STATUS_IS_CHILD_DONE(apr_err))
    return svn_error_wrap_apr(apr_err, _("Error waiting for process '%s'"),
                              cmd);

  if (exitwhy)
    *exitwhy = exitwhy_val;
  else if (! APR_PROC_CHECK_EXIT(exitwhy_val))
    return svn_error_createf
      (SVN_ERR_EXTERNAL_PROGRAM, NULL,
       _("Process '%s' failed (exitwhy %d)"), cmd, exitwhy_val);

  if (exitcode)
    *exitcode = exitcode_val;
  else if (exitcode_val != 0)
    return svn_error_createf
      (SVN_ERR_EXTERNAL_PROGRAM, NULL,
       _("Process '%s' returned error exitcode %d"), cmd, exitcode_val);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_run_cmd(const char *path,
               const char *cmd,
               const char *const *args,
               int *exitcode,
               apr_exit_why_e *exitwhy,
               svn_boolean_t inherit,
               apr_file_t *infile,
               apr_file_t *outfile,
               apr_file_t *errfile,
               apr_pool_t *pool)
{
  apr_proc_t cmd_proc;

  SVN_ERR(svn_io_start_cmd(&cmd_proc, path, cmd, args, inherit,
                           infile, outfile, errfile, pool));

  return svn_io_wait_for_cmd(&cmd_proc, cmd, exitcode, exitwhy, pool);
}


svn_error_t *
svn_io_run_diff2(const char *dir,
                 const char *const *user_args,
                 int num_user_args,
                 const char *label1,
                 const char *label2,
                 const char *from,
                 const char *to,
                 int *pexitcode,
                 apr_file_t *outfile,
                 apr_file_t *errfile,
                 const char *diff_cmd,
                 apr_pool_t *pool)
{
  const char **args;
  int i;
  int exitcode;
  int nargs = 4; /* the diff command itself, two paths, plus a trailing NULL */
  apr_pool_t *subpool = svn_pool_create(pool);

  if (pexitcode == NULL)
    pexitcode = &exitcode;

  if (user_args != NULL)
    nargs += num_user_args;
  else
    nargs += 1; /* -u */

  if (label1 != NULL)
    nargs += 2; /* the -L and the label itself */
  if (label2 != NULL)
    nargs += 2; /* the -L and the label itself */

  args = apr_palloc(subpool, nargs * sizeof(char *));

  i = 0;
  args[i++] = diff_cmd;

  if (user_args != NULL)
    {
      int j;
      for (j = 0; j < num_user_args; ++j)
        args[i++] = user_args[j];
    }
  else
    args[i++] = "-u"; /* assume -u if the user didn't give us any args */

  if (label1 != NULL)
    {
      args[i++] = "-L";
      args[i++] = label1;
    }
  if (label2 != NULL)
    {
      args[i++] = "-L";
      args[i++] = label2;
    }

  args[i++] = svn_path_local_style(from, subpool);
  args[i++] = svn_path_local_style(to, subpool);
  args[i++] = NULL;

  SVN_ERR_ASSERT(i == nargs);

  SVN_ERR(svn_io_run_cmd(dir, diff_cmd, args, pexitcode, NULL, TRUE,
                         NULL, outfile, errfile, subpool));

  /* The man page for (GNU) diff describes the return value as:

       "An exit status of 0 means no differences were found, 1 means
        some differences were found, and 2 means trouble."

     A return value of 2 typically occurs when diff cannot read its input
     or write to its output, but in any case we probably ought to return an
     error for anything other than 0 or 1 as the output is likely to be
     corrupt.
   */
  if (*pexitcode != 0 && *pexitcode != 1)
    return svn_error_createf(SVN_ERR_EXTERNAL_PROGRAM, NULL,
                             _("'%s' returned %d"),
                             svn_path_local_style(diff_cmd, pool),
                             *pexitcode);

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_run_diff3_3(int *exitcode,
                   const char *dir,
                   const char *mine,
                   const char *older,
                   const char *yours,
                   const char *mine_label,
                   const char *older_label,
                   const char *yours_label,
                   apr_file_t *merged,
                   const char *diff3_cmd,
                   const apr_array_header_t *user_args,
                   apr_pool_t *pool)
{
  const char **args = apr_palloc(pool,
                                 sizeof(char*) * (13
                                                  + (user_args
                                                     ? user_args->nelts
                                                     : 1)));
#ifndef NDEBUG
  int nargs = 12;
#endif
  int i = 0;

  /* Labels fall back to sensible defaults if not specified. */
  if (mine_label == NULL)
    mine_label = ".working";
  if (older_label == NULL)
    older_label = ".old";
  if (yours_label == NULL)
    yours_label = ".new";

  /* Set up diff3 command line. */
  args[i++] = diff3_cmd;
  if (user_args)
    {
      int j;
      for (j = 0; j < user_args->nelts; ++j)
        args[i++] = APR_ARRAY_IDX(user_args, j, const char *);
#ifndef NDEBUG
      nargs += user_args->nelts;
#endif
    }
  else
    {
      args[i++] = "-E";             /* We tried "-A" here, but that caused
                                       overlapping identical changes to
                                       conflict.  See issue #682. */
#ifndef NDEBUG
      ++nargs;
#endif
    }
  args[i++] = "-m";
  args[i++] = "-L";
  args[i++] = mine_label;
  args[i++] = "-L";
  args[i++] = older_label;      /* note:  this label is ignored if
                                   using 2-part markers, which is the
                                   case with "-E". */
  args[i++] = "-L";
  args[i++] = yours_label;
#ifdef SVN_DIFF3_HAS_DIFF_PROGRAM_ARG
  {
    svn_boolean_t has_arg;

    /* ### FIXME: we really shouldn't be reading the config here;
       instead, the necessary bits should be passed in by the caller.
       But should we add another parameter to this function, when the
       whole external diff3 thing might eventually go away?  */
    apr_hash_t *config;
    svn_config_t *cfg;

    SVN_ERR(svn_config_get_config(&config, pool));
    cfg = config ? apr_hash_get(config, SVN_CONFIG_CATEGORY_CONFIG,
                                APR_HASH_KEY_STRING) : NULL;
    SVN_ERR(svn_config_get_bool(cfg, &has_arg, SVN_CONFIG_SECTION_HELPERS,
                                SVN_CONFIG_OPTION_DIFF3_HAS_PROGRAM_ARG,
                                TRUE));
    if (has_arg)
      {
        const char *diff_cmd, *diff_utf8;
        svn_config_get(cfg, &diff_cmd, SVN_CONFIG_SECTION_HELPERS,
                       SVN_CONFIG_OPTION_DIFF_CMD, SVN_CLIENT_DIFF);
        SVN_ERR(cstring_to_utf8(&diff_utf8, diff_cmd, pool));
        args[i++] = apr_pstrcat(pool, "--diff-program=", diff_utf8, NULL);
#ifndef NDEBUG
        ++nargs;
#endif
      }
  }
#endif
  args[i++] = svn_path_local_style(mine, pool);
  args[i++] = svn_path_local_style(older, pool);
  args[i++] = svn_path_local_style(yours, pool);
  args[i++] = NULL;
#ifndef NDEBUG
  SVN_ERR_ASSERT(i == nargs);
#endif

  /* Run diff3, output the merged text into the scratch file. */
  SVN_ERR(svn_io_run_cmd(dir, diff3_cmd, args,
                         exitcode, NULL,
                         TRUE, /* keep environment */
                         NULL, merged, NULL,
                         pool));

  /* According to the diff3 docs, a '0' means the merge was clean, and
     '1' means conflict markers were found.  Anything else is real
     error. */
  if ((*exitcode != 0) && (*exitcode != 1))
    return svn_error_createf(SVN_ERR_EXTERNAL_PROGRAM, NULL,
                             _("Error running '%s':  exitcode was %d, "
                               "args were:"
                               "\nin directory '%s', basenames:\n%s\n%s\n%s"),
                             svn_path_local_style(diff3_cmd, pool),
                             *exitcode,
                             svn_path_local_style(dir, pool),
                             /* Don't call svn_path_local_style() on
                                the basenames.  We don't want them to
                                be absolute, and we don't need the
                                separator conversion. */
                             mine, older, yours);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_io_parse_mimetypes_file(apr_hash_t **type_map,
                            const char *mimetypes_file,
                            apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  apr_hash_t *types = apr_hash_make(pool);
  svn_boolean_t eof = FALSE;
  svn_stringbuf_t *buf;
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_file_t *types_file;
  svn_stream_t *mimetypes_stream;

  SVN_ERR(svn_io_file_open(&types_file, mimetypes_file,
                           APR_READ, APR_OS_DEFAULT, pool));
  mimetypes_stream = svn_stream_from_aprfile2(types_file, FALSE, pool);

  while (1)
    {
      apr_array_header_t *tokens;
      const char *type;
      int i;

      svn_pool_clear(subpool);

      /* Read a line. */
      if ((err = svn_stream_readline(mimetypes_stream, &buf,
                                     APR_EOL_STR, &eof, subpool)))
        break;

      /* Only pay attention to non-empty, non-comment lines. */
      if (buf->len)
        {
          if (buf->data[0] == '#')
            continue;

          /* Tokenize (into our return pool). */
          tokens = svn_cstring_split(buf->data, " \t", TRUE, pool);
          if (tokens->nelts < 2)
            continue;

          /* The first token in a multi-token line is the media type.
             Subsequent tokens are filename extensions associated with
             that media type. */
          type = APR_ARRAY_IDX(tokens, 0, const char *);
          for (i = 1; i < tokens->nelts; i++)
            {
              const char *ext = APR_ARRAY_IDX(tokens, i, const char *);
              apr_hash_set(types, ext, APR_HASH_KEY_STRING, type);
            }
        }
      if (eof)
        break;
    }
  svn_pool_destroy(subpool);

  /* If there was an error above, close the file (ignoring any error
     from *that*) and return the originally error. */
  if (err)
    {
      svn_error_clear(svn_stream_close(mimetypes_stream));
      return err;
    }

  /* Close the stream (which closes the underlying file, too). */
  SVN_ERR(svn_stream_close(mimetypes_stream));

  *type_map = types;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_detect_mimetype2(const char **mimetype,
                        const char *file,
                        apr_hash_t *mimetype_map,
                        apr_pool_t *pool)
{
  static const char * const generic_binary = "application/octet-stream";

  svn_node_kind_t kind;
  apr_file_t *fh;
  svn_error_t *err;
  unsigned char block[1024];
  apr_size_t amt_read = sizeof(block);

  /* Default return value is NULL. */
  *mimetype = NULL;

  /* See if this file even exists, and make sure it really is a file. */
  SVN_ERR(svn_io_check_path(file, &kind, pool));
  if (kind != svn_node_file)
    return svn_error_createf(SVN_ERR_BAD_FILENAME, NULL,
                             _("Can't detect MIME type of non-file '%s'"),
                             svn_path_local_style(file, pool));

  /* If there is a mimetype_map provided, we'll first try to look up
     our file's extension in the map.  Failing that, we'll run the
     heuristic. */
  if (mimetype_map)
    {
      const char *type_from_map, *path_ext;
      svn_path_splitext(NULL, &path_ext, file, pool);
      if ((type_from_map = apr_hash_get(mimetype_map, path_ext,
                                        APR_HASH_KEY_STRING)))
        {
          *mimetype = type_from_map;
          return SVN_NO_ERROR;
        }
    }

  SVN_ERR(svn_io_file_open(&fh, file, APR_READ, 0, pool));

  /* Read a block of data from FILE. */
  err = svn_io_file_read(fh, block, &amt_read, pool);
  if (err && ! APR_STATUS_IS_EOF(err->apr_err))
    return err;
  svn_error_clear(err);

  /* Now close the file.  No use keeping it open any more.  */
  SVN_ERR(svn_io_file_close(fh, pool));


  /* Right now, this function is going to be really stupid.  It's
     going to examine the first block of data, and make sure that 85%
     of the bytes are such that their value is in the ranges 0x07-0x0D
     or 0x20-0x7F, and that 100% of those bytes is not 0x00.

     If those criteria are not met, we're calling it binary. */
  if (amt_read > 0)
    {
      apr_size_t i;
      int binary_count = 0;

      /* Run through the data we've read, counting the 'binary-ish'
         bytes.  HINT: If we see a 0x00 byte, we'll set our count to its
         max and stop reading the file. */
      for (i = 0; i < amt_read; i++)
        {
          if (block[i] == 0)
            {
              binary_count = amt_read;
              break;
            }
          if ((block[i] < 0x07)
              || ((block[i] > 0x0D) && (block[i] < 0x20))
              || (block[i] > 0x7F))
            {
              binary_count++;
            }
        }

      if (((binary_count * 1000) / amt_read) > 850)
        {
          *mimetype = generic_binary;
          return SVN_NO_ERROR;
        }
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_detect_mimetype(const char **mimetype,
                       const char *file,
                       apr_pool_t *pool)
{
  return svn_io_detect_mimetype2(mimetype, file, NULL, pool);
}


svn_error_t *
svn_io_file_open(apr_file_t **new_file, const char *fname,
                 apr_int32_t flag, apr_fileperms_t perm,
                 apr_pool_t *pool)
{
  const char *fname_apr;
  apr_status_t status;

  SVN_ERR(cstring_from_utf8(&fname_apr, fname, pool));
  status = file_open(new_file, fname_apr, flag | APR_BINARY, perm, TRUE,
                     pool);

  if (status)
    return svn_error_wrap_apr(status, _("Can't open file '%s'"),
                              svn_path_local_style(fname, pool));
  else
    return SVN_NO_ERROR;
}


static svn_error_t *
do_io_file_wrapper_cleanup(apr_file_t *file, apr_status_t status,
                           const char *msg, const char *msg_no_name,
                           apr_pool_t *pool)
{
  const char *name;
  svn_error_t *err;

  if (! status)
    return SVN_NO_ERROR;

  err = file_name_get(&name, file, pool);
  if (err)
    name = NULL;
  svn_error_clear(err);

  if (name)
    return svn_error_wrap_apr(status, _(msg),
                              svn_path_local_style(name, pool));
  else
    return svn_error_wrap_apr(status, _(msg_no_name));
}


svn_error_t *
svn_io_file_close(apr_file_t *file, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_close(file),
     N_("Can't close file '%s'"),
     N_("Can't close stream"),
      pool);
}


svn_error_t *
svn_io_file_getc(char *ch, apr_file_t *file, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_getc(ch, file),
     N_("Can't read file '%s'"),
     N_("Can't read stream"),
     pool);
}


svn_error_t *
svn_io_file_info_get(apr_finfo_t *finfo, apr_int32_t wanted,
                     apr_file_t *file, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_info_get(finfo, wanted, file),
     N_("Can't get attribute information from file '%s'"),
     N_("Can't get attribute information from stream"),
     pool);
}


svn_error_t *
svn_io_file_read(apr_file_t *file, void *buf,
                 apr_size_t *nbytes, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_read(file, buf, nbytes),
     N_("Can't read file '%s'"),
     N_("Can't read stream"),
     pool);
}


svn_error_t *
svn_io_file_read_full(apr_file_t *file, void *buf,
                      apr_size_t nbytes, apr_size_t *bytes_read,
                      apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_read_full(file, buf, nbytes, bytes_read),
     N_("Can't read file '%s'"),
     N_("Can't read stream"),
     pool);
}


svn_error_t *
svn_io_file_seek(apr_file_t *file, apr_seek_where_t where,
                 apr_off_t *offset, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_seek(file, where, offset),
     N_("Can't set position pointer in file '%s'"),
     N_("Can't set position pointer in stream"),
     pool);
}


svn_error_t *
svn_io_file_write(apr_file_t *file, const void *buf,
                  apr_size_t *nbytes, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_write(file, buf, nbytes),
     N_("Can't write to file '%s'"),
     N_("Can't write to stream"),
     pool);
}


svn_error_t *
svn_io_file_write_full(apr_file_t *file, const void *buf,
                       apr_size_t nbytes, apr_size_t *bytes_written,
                       apr_pool_t *pool)
{
  apr_status_t rv = apr_file_write_full(file, buf, nbytes, bytes_written);

#ifdef WIN32
#define MAXBUFSIZE 30*1024
  if (rv == APR_FROM_OS_ERROR(ERROR_NOT_ENOUGH_MEMORY)
      && nbytes > MAXBUFSIZE)
    {
      apr_size_t bw = 0;
      *bytes_written = 0;

      do {
        rv = apr_file_write_full(file, buf,
                                 nbytes > MAXBUFSIZE ? MAXBUFSIZE : nbytes, &bw);
        *bytes_written += bw;
        buf = (char *)buf + bw;
        nbytes -= bw;
      } while (rv == APR_SUCCESS && nbytes > 0);
    }
#undef MAXBUFSIZE
#endif

  return do_io_file_wrapper_cleanup
    (file, rv,
     N_("Can't write to file '%s'"),
     N_("Can't write to stream"),
     pool);
}


svn_error_t *
svn_io_write_unique(const char **tmp_path,
                    const char *dirpath,
                    const void *buf,
                    apr_size_t nbytes,
                    svn_io_file_del_t delete_when,
                    apr_pool_t *pool)
{
  apr_file_t *new_file;

  SVN_ERR(svn_io_open_unique_file3(&new_file, tmp_path, dirpath,
                                   delete_when, pool, pool));
  SVN_ERR(svn_io_file_write_full(new_file, buf, nbytes, NULL, pool));
  SVN_ERR(svn_io_file_flush_to_disk(new_file, pool));
  return svn_io_file_close(new_file, pool);
}


svn_error_t *
svn_io_file_trunc(apr_file_t *file, apr_off_t offset, apr_pool_t *pool)
{
  return do_io_file_wrapper_cleanup
    (file, apr_file_trunc(file, offset),
     N_("Can't truncate file '%s'"),
     N_("Can't truncate stream"),
     pool);
}


svn_error_t *
svn_io_read_length_line(apr_file_t *file, char *buf, apr_size_t *limit,
                        apr_pool_t *pool)
{
  const char *name;
  svn_error_t *err;
  apr_size_t i;
  char c;

  for (i = 0; i < *limit; i++)
    {
      SVN_ERR(svn_io_file_getc(&c, file, pool));
      /* Note: this error could be APR_EOF, which
         is totally fine.  The caller should be aware of
         this. */

      if (c == '\n')
        {
          buf[i] = '\0';
          *limit = i;
          return SVN_NO_ERROR;
        }
      else
        {
          buf[i] = c;
        }
    }

  err = file_name_get(&name, file, pool);
  if (err)
    name = NULL;
  svn_error_clear(err);

  if (name)
    return svn_error_createf(SVN_ERR_MALFORMED_FILE, NULL,
                             _("Can't read length line in file '%s'"),
                             svn_path_local_style(name, pool));
  else
    return svn_error_create(SVN_ERR_MALFORMED_FILE, NULL,
                            _("Can't read length line in stream"));
}


svn_error_t *
svn_io_stat(apr_finfo_t *finfo, const char *fname,
            apr_int32_t wanted, apr_pool_t *pool)
{
  apr_status_t status;
  const char *fname_apr;

  /* APR doesn't like "" directories */
  if (fname[0] == '\0')
    fname = ".";

  SVN_ERR(cstring_from_utf8(&fname_apr, fname, pool));

  status = apr_stat(finfo, fname_apr, wanted, pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't stat '%s'"),
                              svn_path_local_style(fname, pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_file_rename(const char *from_path, const char *to_path,
                   apr_pool_t *pool)
{
  apr_status_t status = APR_SUCCESS;
  const char *from_path_apr, *to_path_apr;

  SVN_ERR(cstring_from_utf8(&from_path_apr, from_path, pool));
  SVN_ERR(cstring_from_utf8(&to_path_apr, to_path, pool));

  status = apr_file_rename(from_path_apr, to_path_apr, pool);

#ifdef WIN32
  if (status)
    {
      /* Set the destination file writable because Windows will not
         allow us to rename over files that are read-only. */
      SVN_ERR(svn_io_set_file_read_write(to_path, TRUE, pool));

      status = apr_file_rename(from_path_apr, to_path_apr, pool);

      WIN32_RETRY_LOOP(status,
                       apr_file_rename(from_path_apr, to_path_apr, pool));
    }
#endif /* WIN32 */

  if (status)
    return svn_error_wrap_apr(status, _("Can't move '%s' to '%s'"),
                              svn_path_local_style(from_path, pool),
                              svn_path_local_style(to_path, pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_file_move(const char *from_path, const char *to_path,
                 apr_pool_t *pool)
{
  svn_error_t *err = svn_io_file_rename(from_path, to_path, pool);

  if (err && APR_STATUS_IS_EXDEV(err->apr_err))
    {
      const char *tmp_to_path;

      svn_error_clear(err);

      SVN_ERR(svn_io_open_unique_file3(NULL, &tmp_to_path,
                                       svn_path_dirname(to_path, pool),
                                       svn_io_file_del_none,
                                       pool, pool));

      err = svn_io_copy_file(from_path, tmp_to_path, TRUE, pool);
      if (err)
        goto failed_tmp;

      err = svn_io_file_rename(tmp_to_path, to_path, pool);
      if (err)
        goto failed_tmp;

      err = svn_io_remove_file(from_path, pool);
      if (! err)
        return SVN_NO_ERROR;

      svn_error_clear(svn_io_remove_file(to_path, pool));

      return err;

    failed_tmp:
      svn_error_clear(svn_io_remove_file(tmp_to_path, pool));
    }

  return err;
}

/* Common implementation of svn_io_dir_make and svn_io_dir_make_hidden.
   HIDDEN determines if the hidden attribute
   should be set on the newly created directory. */
static svn_error_t *
dir_make(const char *path, apr_fileperms_t perm,
         svn_boolean_t hidden, svn_boolean_t sgid, apr_pool_t *pool)
{
  apr_status_t status;
  const char *path_apr;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  /* APR doesn't like "" directories */
  if (path_apr[0] == '\0')
    path_apr = ".";

#if (APR_OS_DEFAULT & APR_WSTICKY)
  /* The APR shipped with httpd 2.0.50 contains a bug where
     APR_OS_DEFAULT encompasses the setuid, setgid, and sticky bits.
     There is a special case for file creation, but not directory
     creation, so directories wind up getting created with the sticky
     bit set.  (There is no such thing as a setuid directory, and the
     setgid bit is apparently ignored at mkdir() time.)  If we detect
     this problem, work around it by unsetting those bits if we are
     passed APR_OS_DEFAULT. */
  if (perm == APR_OS_DEFAULT)
    perm &= ~(APR_USETID | APR_GSETID | APR_WSTICKY);
#endif

  status = apr_dir_make(path_apr, perm, pool);
  WIN32_RETRY_LOOP(status, apr_dir_make(path_apr, perm, pool));

  if (status)
    return svn_error_wrap_apr(status, _("Can't create directory '%s'"),
                              svn_path_local_style(path, pool));

#ifdef APR_FILE_ATTR_HIDDEN
  if (hidden)
    {
      status = apr_file_attrs_set(path_apr,
                                  APR_FILE_ATTR_HIDDEN,
                                  APR_FILE_ATTR_HIDDEN,
                                  pool);
      if (status)
        return svn_error_wrap_apr(status, _("Can't hide directory '%s'"),
                                  svn_path_local_style(path, pool));
    }
#endif

/* Windows does not implement sgid. Skip here because retrieving
   the file permissions via APR_FINFO_PROT | APR_FINFO_OWNER is documented
   to be 'incredibly expensive'. */
#ifndef WIN32
  if (sgid)
    {
      apr_finfo_t finfo;

      /* Per our contract, don't do error-checking.  Some filesystems
       * don't support the sgid bit, and that's okay. */
      status = apr_stat(&finfo, path_apr, APR_FINFO_PROT, pool);

      if (!status)
        apr_file_perms_set(path_apr, finfo.protection | APR_GSETID);
    }
#endif

  return SVN_NO_ERROR;
}

svn_error_t *
svn_io_dir_make(const char *path, apr_fileperms_t perm, apr_pool_t *pool)
{
  return dir_make(path, perm, FALSE, FALSE, pool);
}

svn_error_t *
svn_io_dir_make_hidden(const char *path, apr_fileperms_t perm,
                       apr_pool_t *pool)
{
  return dir_make(path, perm, TRUE, FALSE, pool);
}

svn_error_t *
svn_io_dir_make_sgid(const char *path, apr_fileperms_t perm,
                     apr_pool_t *pool)
{
  return dir_make(path, perm, FALSE, TRUE, pool);
}


svn_error_t *
svn_io_dir_open(apr_dir_t **new_dir, const char *dirname, apr_pool_t *pool)
{
  apr_status_t status;
  const char *dirname_apr;

  /* APR doesn't like "" directories */
  if (dirname[0] == '\0')
    dirname = ".";

  SVN_ERR(cstring_from_utf8(&dirname_apr, dirname, pool));

  status = apr_dir_open(new_dir, dirname_apr, pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't open directory '%s'"),
                              svn_path_local_style(dirname, pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_dir_remove_nonrecursive(const char *dirname, apr_pool_t *pool)
{
  apr_status_t status;
  const char *dirname_apr;

  SVN_ERR(cstring_from_utf8(&dirname_apr, dirname, pool));

  status = apr_dir_remove(dirname_apr, pool);
  WIN32_RETRY_LOOP(status, apr_dir_remove(dirname_apr, pool));
  if (status)
    return svn_error_wrap_apr(status, _("Can't remove directory '%s'"),
                              svn_path_local_style(dirname, pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_dir_read(apr_finfo_t *finfo,
                apr_int32_t wanted,
                apr_dir_t *thedir,
                apr_pool_t *pool)
{
  apr_status_t status;

  status = apr_dir_read(finfo, wanted, thedir);

  if (status)
    return svn_error_wrap_apr(status, _("Can't read directory"));

  /* It would be nice to use entry_name_to_utf8() below, but can we
     get the dir's path out of an apr_dir_t?  I don't see a reliable
     way to do it. */

  if (finfo->fname)
    SVN_ERR(svn_path_cstring_to_utf8(&finfo->fname, finfo->fname, pool));

  if (finfo->name)
    SVN_ERR(svn_path_cstring_to_utf8(&finfo->name, finfo->name, pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_io_dir_walk(const char *dirname,
                apr_int32_t wanted,
                svn_io_walk_func_t walk_func,
                void *walk_baton,
                apr_pool_t *pool)
{
  apr_status_t apr_err;
  apr_dir_t *handle;
  apr_pool_t *subpool;
  const char *dirname_apr;
  apr_finfo_t finfo;

  wanted |= APR_FINFO_TYPE | APR_FINFO_NAME;

  /* The documentation for apr_dir_read used to state that "." and ".."
     will be returned as the first two files, but it doesn't
     work that way in practice, in particular ext3 on Linux-2.6 doesn't
     follow the rules.  For details see
     http://subversion.tigris.org/servlets/ReadMsg?list=dev&msgNo=56666

     If APR ever does implement "dot-first" then it would be possible to
     remove the svn_io_stat and walk_func calls and use the walk_func
     inside the loop.

     Note: apr_stat doesn't handle FINFO_NAME but svn_io_dir_walk is
     documented to provide it, so we have to do a bit extra. */
  SVN_ERR(svn_io_stat(&finfo, dirname, wanted & ~APR_FINFO_NAME, pool));
  SVN_ERR(cstring_from_utf8(&finfo.name,
                            svn_path_basename(dirname, pool),
                            pool));
  finfo.valid |= APR_FINFO_NAME;
  SVN_ERR((*walk_func)(walk_baton, dirname, &finfo, pool));

  SVN_ERR(cstring_from_utf8(&dirname_apr, dirname, pool));

  apr_err = apr_dir_open(&handle, dirname_apr, pool);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't open directory '%s'"),
                              svn_path_local_style(dirname, pool));

  /* iteration subpool */
  subpool = svn_pool_create(pool);

  while (1)
    {
      const char *name_utf8;
      const char *full_path;

      svn_pool_clear(subpool);

      apr_err = apr_dir_read(&finfo, wanted, handle);
      if (APR_STATUS_IS_ENOENT(apr_err))
        break;
      else if (apr_err)
        {
          return svn_error_wrap_apr
            (apr_err, _("Can't read directory entry in '%s'"),
             svn_path_local_style(dirname, pool));
        }

      if (finfo.filetype == APR_DIR)
        {
          if (finfo.name[0] == '.'
              && (finfo.name[1] == '\0'
                  || (finfo.name[1] == '.' && finfo.name[2] == '\0')))
            /* skip "." and ".." */
            continue;

          /* some other directory. recurse. it will be passed to the
             callback inside the recursion. */
          SVN_ERR(entry_name_to_utf8(&name_utf8, finfo.name, dirname,
                                     subpool));
          full_path = svn_path_join(dirname, name_utf8, subpool);
          SVN_ERR(svn_io_dir_walk(full_path,
                                  wanted,
                                  walk_func,
                                  walk_baton,
                                  subpool));
        }
      else if (finfo.filetype == APR_REG)
        {
          /* some other directory. pass it to the callback. */
          SVN_ERR(entry_name_to_utf8(&name_utf8, finfo.name, dirname,
                                     subpool));
          full_path = svn_path_join(dirname, name_utf8, subpool);
          SVN_ERR((*walk_func)(walk_baton,
                               full_path,
                               &finfo,
                               subpool));
        }
      /* else:
         some other type of file; skip it.
      */

    }

  svn_pool_destroy(subpool);

  apr_err = apr_dir_close(handle);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Error closing directory '%s'"),
                              svn_path_local_style(dirname, pool));

  return SVN_NO_ERROR;
}



/**
 * Determine if a directory is empty or not.
 * @param Return APR_SUCCESS if the dir is empty, else APR_ENOTEMPTY if not.
 * @param path The directory.
 * @param pool Used for temporary allocation.
 * @remark If path is not a directory, or some other error occurs,
 * then return the appropriate apr status code.
 *
 * (This function is written in APR style, in anticipation of
 * perhaps someday being moved to APR as 'apr_dir_is_empty'.)
 */
static apr_status_t
dir_is_empty(const char *dir, apr_pool_t *pool)
{
  apr_status_t apr_err;
  apr_dir_t *dir_handle;
  apr_finfo_t finfo;
  apr_status_t retval = APR_SUCCESS;

  /* APR doesn't like "" directories */
  if (dir[0] == '\0')
    dir = ".";

  apr_err = apr_dir_open(&dir_handle, dir, pool);
  if (apr_err != APR_SUCCESS)
    return apr_err;

  for (apr_err = apr_dir_read(&finfo, APR_FINFO_NAME, dir_handle);
       apr_err == APR_SUCCESS;
       apr_err = apr_dir_read(&finfo, APR_FINFO_NAME, dir_handle))
    {
      /* Ignore entries for this dir and its parent, robustly.
         (APR promises that they'll come first, so technically
         this guard could be moved outside the loop.  But Ryan Bloom
         says he doesn't believe it, and I believe him. */
      if (! (finfo.name[0] == '.'
             && (finfo.name[1] == '\0'
                 || (finfo.name[1] == '.' && finfo.name[2] == '\0'))))
        {
          retval = APR_ENOTEMPTY;
          break;
        }
    }

  /* Make sure we broke out of the loop for the right reason. */
  if (apr_err && ! APR_STATUS_IS_ENOENT(apr_err))
    return apr_err;

  apr_err = apr_dir_close(dir_handle);
  if (apr_err != APR_SUCCESS)
    return apr_err;

  return retval;
}


svn_error_t *
svn_io_dir_empty(svn_boolean_t *is_empty_p,
                 const char *path,
                 apr_pool_t *pool)
{
  apr_status_t status;
  const char *path_apr;

  SVN_ERR(cstring_from_utf8(&path_apr, path, pool));

  status = dir_is_empty(path_apr, pool);

  if (!status)
    *is_empty_p = TRUE;
  else if (APR_STATUS_IS_ENOTEMPTY(status))
    *is_empty_p = FALSE;
  else
    return svn_error_wrap_apr(status, _("Can't check directory '%s'"),
                              svn_path_local_style(path, pool));

  return SVN_NO_ERROR;
}



/*** Version/format files ***/

svn_error_t *
svn_io_write_version_file(const char *path,
                          int version,
                          apr_pool_t *pool)
{
  const char *path_tmp;
  const char *format_contents = apr_psprintf(pool, "%d\n", version);

  SVN_ERR_ASSERT(version >= 0);

  SVN_ERR(svn_io_write_unique(&path_tmp,
                              svn_path_dirname(path, pool),
                              format_contents, strlen(format_contents),
                              svn_io_file_del_none, pool));

#ifdef WIN32
  /* make the destination writable, but only on Windows, because
     Windows does not let us replace read-only files. */
  SVN_ERR(svn_io_set_file_read_write(path, TRUE, pool));
#endif /* WIN32 */

  /* rename the temp file as the real destination */
  SVN_ERR(svn_io_file_rename(path_tmp, path, pool));

  /* And finally remove the perms to make it read only */
  return svn_io_set_file_read_only(path, FALSE, pool);
}


svn_error_t *
svn_io_read_version_file(int *version,
                         const char *path,
                         apr_pool_t *pool)
{
  apr_file_t *format_file;
  char buf[80];
  apr_size_t len;

  /* Read a chunk of data from PATH */
  SVN_ERR(svn_io_file_open(&format_file, path, APR_READ,
                           APR_OS_DEFAULT, pool));
  len = sizeof(buf);
  SVN_ERR(svn_io_file_read(format_file, buf, &len, pool));

  /* Close the file. */
  SVN_ERR(svn_io_file_close(format_file, pool));

  /* If there was no data in PATH, return an error. */
  if (len == 0)
    return svn_error_createf(SVN_ERR_STREAM_UNEXPECTED_EOF, NULL,
                             _("Reading '%s'"),
                             svn_path_local_style(path, pool));

  /* Check that the first line contains only digits. */
  {
    apr_size_t i;

    for (i = 0; i < len; ++i)
      {
        char c = buf[i];

        if (i > 0 && (c == '\r' || c == '\n'))
          break;
        if (! apr_isdigit(c))
          return svn_error_createf
            (SVN_ERR_BAD_VERSION_FILE_FORMAT, NULL,
             _("First line of '%s' contains non-digit"),
             svn_path_local_style(path, pool));
      }
  }

  /* Convert to integer. */
  *version = atoi(buf);

  return SVN_NO_ERROR;
}



/* Do a byte-for-byte comparison of FILE1 and FILE2. */
static svn_error_t *
contents_identical_p(svn_boolean_t *identical_p,
                     const char *file1,
                     const char *file2,
                     apr_pool_t *pool)
{
  svn_error_t *err1;
  svn_error_t *err2;
  apr_size_t bytes_read1, bytes_read2;
  char *buf1 = apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);
  char *buf2 = apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);
  apr_file_t *file1_h = NULL;
  apr_file_t *file2_h = NULL;

  SVN_ERR(svn_io_file_open(&file1_h, file1, APR_READ, APR_OS_DEFAULT,
                           pool));
  SVN_ERR(svn_io_file_open(&file2_h, file2, APR_READ, APR_OS_DEFAULT,
                           pool));

  *identical_p = TRUE;  /* assume TRUE, until disproved below */
  do
    {
      err1 = svn_io_file_read_full(file1_h, buf1,
                                   SVN__STREAM_CHUNK_SIZE, &bytes_read1, pool);
      if (err1 && !APR_STATUS_IS_EOF(err1->apr_err))
        return err1;

      err2 = svn_io_file_read_full(file2_h, buf2,
                                   SVN__STREAM_CHUNK_SIZE, &bytes_read2, pool);
      if (err2 && !APR_STATUS_IS_EOF(err2->apr_err))
        {
          svn_error_clear(err1);
          return err2;
        }

      if ((bytes_read1 != bytes_read2)
          || (memcmp(buf1, buf2, bytes_read1)))
        {
          *identical_p = FALSE;
          break;
        }
    } while (! err1 && ! err2);

  svn_error_clear(err1);
  svn_error_clear(err2);

  SVN_ERR(svn_io_file_close(file1_h, pool));
  return svn_io_file_close(file2_h, pool);
}



svn_error_t *
svn_io_files_contents_same_p(svn_boolean_t *same,
                             const char *file1,
                             const char *file2,
                             apr_pool_t *pool)
{
  svn_boolean_t q;

  SVN_ERR(svn_io_filesizes_different_p(&q, file1, file2, pool));

  if (q)
    {
      *same = 0;
      return SVN_NO_ERROR;
    }

  SVN_ERR(contents_identical_p(&q, file1, file2, pool));

  if (q)
    *same = 1;
  else
    *same = 0;

  return SVN_NO_ERROR;
}
