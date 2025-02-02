/*
 * config_auth.c :  authentication files in the user config area
 *
 * ====================================================================
 * Copyright (c) 2000-2004, 2008 CollabNet.  All rights reserved.
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



#include "svn_path.h"
#include "svn_hash.h"
#include "svn_io.h"

#include "config_impl.h"

#include "svn_private_config.h"

/* Helper for svn_config_{read|write}_auth_data.  Return a path to a
   file within ~/.subversion/auth/ that holds CRED_KIND credentials
   within REALMSTRING.  If no path is available *PATH will be set to
   NULL. */
static svn_error_t *
auth_file_path(const char **path,
               const char *cred_kind,
               const char *realmstring,
               const char *config_dir,
               apr_pool_t *pool)
{
  const char *authdir_path, *hexname;
  svn_checksum_t *checksum;

  /* Construct the path to the directory containing the creds files,
     e.g. "~/.subversion/auth/svn.simple".  The last component is
     simply the cred_kind.  */
  SVN_ERR(svn_config_get_user_config_path(&authdir_path, config_dir,
                                          SVN_CONFIG__AUTH_SUBDIR, pool));
  if (authdir_path)
    {
      authdir_path = svn_path_join(authdir_path, cred_kind, pool);

      /* Construct the basename of the creds file.  It's just the
         realmstring converted into an md5 hex string.  */
      SVN_ERR(svn_checksum(&checksum, svn_checksum_md5, realmstring,
                           strlen(realmstring), pool));
      hexname = svn_checksum_to_cstring(checksum, pool);

      *path = svn_path_join(authdir_path, hexname, pool);
    }
  else
    *path = NULL;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_config_read_auth_data(apr_hash_t **hash,
                          const char *cred_kind,
                          const char *realmstring,
                          const char *config_dir,
                          apr_pool_t *pool)
{
  svn_node_kind_t kind;
  const char *auth_path;

  *hash = NULL;

  SVN_ERR(auth_file_path(&auth_path, cred_kind, realmstring, config_dir,
                         pool));
  if (! auth_path)
    return SVN_NO_ERROR;

  SVN_ERR(svn_io_check_path(auth_path, &kind, pool));
  if (kind == svn_node_file)
    {
      svn_stream_t *stream;

      SVN_ERR_W(svn_stream_open_readonly(&stream, auth_path, pool, pool),
                _("Unable to open auth file for reading"));

      *hash = apr_hash_make(pool);

      SVN_ERR_W(svn_hash_read2(*hash, stream, SVN_HASH_TERMINATOR, pool),
                apr_psprintf(pool, _("Error parsing '%s'"),
                             svn_path_local_style(auth_path, pool)));

      SVN_ERR(svn_stream_close(stream));
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_config_write_auth_data(apr_hash_t *hash,
                           const char *cred_kind,
                           const char *realmstring,
                           const char *config_dir,
                           apr_pool_t *pool)
{
  apr_file_t *authfile = NULL;
  svn_stream_t *stream;
  const char *auth_path;

  SVN_ERR(auth_file_path(&auth_path, cred_kind, realmstring, config_dir,
                         pool));
  if (! auth_path)
    return svn_error_create(SVN_ERR_NO_AUTH_FILE_PATH, NULL,
                            _("Unable to locate auth file"));

  /* Add the realmstring to the hash, so programs (or users) can
     verify exactly which set of credentials this file holds.  */
  apr_hash_set(hash, SVN_CONFIG_REALMSTRING_KEY, APR_HASH_KEY_STRING,
               svn_string_create(realmstring, pool));

  SVN_ERR_W(svn_io_file_open(&authfile, auth_path,
                             (APR_WRITE | APR_CREATE | APR_TRUNCATE
                              | APR_BUFFERED),
                             APR_OS_DEFAULT, pool),
            _("Unable to open auth file for writing"));

  stream = svn_stream_from_aprfile2(authfile, FALSE, pool);
  SVN_ERR_W(svn_hash_write2(hash, stream, SVN_HASH_TERMINATOR, pool),
            apr_psprintf(pool, _("Error writing hash to '%s'"),
                         svn_path_local_style(auth_path, pool)));

  SVN_ERR(svn_stream_close(stream));

  /* To be nice, remove the realmstring from the hash again, just in
     case the caller wants their hash unchanged. */
  apr_hash_set(hash, SVN_CONFIG_REALMSTRING_KEY, APR_HASH_KEY_STRING, NULL);

  return SVN_NO_ERROR;
}
