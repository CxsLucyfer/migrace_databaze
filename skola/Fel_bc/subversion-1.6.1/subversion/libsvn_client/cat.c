/*
 * cat.c:  implementation of the 'cat' command
 *
 * ====================================================================
 * Copyright (c) 2000-2007 CollabNet.  All rights reserved.
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

/* ==================================================================== */



/*** Includes. ***/

#include "svn_client.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_subst.h"
#include "svn_io.h"
#include "svn_time.h"
#include "svn_path.h"
#include "svn_props.h"
#include "client.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/*** Code. ***/

/* Helper function to handle copying a potentially translated version of
   local file PATH to OUTPUT.  REVISION must be one of the following: BASE,
   COMMITTED, WORKING.  Uses POOL for temporary allocations. */
static svn_error_t *
cat_local_file(const char *path,
               svn_stream_t *output,
               svn_wc_adm_access_t *adm_access,
               const svn_opt_revision_t *revision,
               svn_cancel_func_t cancel_func,
               void *cancel_baton,
               apr_pool_t *pool)
{
  const svn_wc_entry_t *entry;
  apr_hash_t *kw = NULL;
  svn_subst_eol_style_t style;
  apr_hash_t *props;
  svn_string_t *eol_style, *keywords, *special;
  const char *eol = NULL;
  svn_boolean_t local_mod = FALSE;
  apr_time_t tm;
  svn_stream_t *input;

  SVN_ERR_ASSERT(SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind));

  SVN_ERR(svn_wc__entry_versioned(&entry, path, adm_access, FALSE, pool));

  if (entry->kind != svn_node_file)
    return svn_error_createf(SVN_ERR_CLIENT_IS_DIRECTORY, NULL,
                             _("'%s' refers to a directory"),
                             svn_path_local_style(path, pool));

  if (revision->kind != svn_opt_revision_working)
    {
      SVN_ERR(svn_wc_get_pristine_contents(&input, path, pool, pool));
      SVN_ERR(svn_wc_get_prop_diffs(NULL, &props, path, adm_access, pool));
    }
  else
    {
      svn_wc_status2_t *status;

      SVN_ERR(svn_stream_open_readonly(&input, path, pool, pool));

      SVN_ERR(svn_wc_prop_list(&props, path, adm_access, pool));
      SVN_ERR(svn_wc_status2(&status, path, adm_access, pool));
      if (status->text_status != svn_wc_status_normal)
        local_mod = TRUE;
    }

  eol_style = apr_hash_get(props, SVN_PROP_EOL_STYLE,
                           APR_HASH_KEY_STRING);
  keywords = apr_hash_get(props, SVN_PROP_KEYWORDS,
                          APR_HASH_KEY_STRING);
  special = apr_hash_get(props, SVN_PROP_SPECIAL,
                         APR_HASH_KEY_STRING);

  if (eol_style)
    svn_subst_eol_style_from_value(&style, &eol, eol_style->data);

  if (local_mod && (! special))
    {
      /* Use the modified time from the working copy if
         the file */
      SVN_ERR(svn_io_file_affected_time(&tm, path, pool));
    }
  else
    {
      tm = entry->cmt_date;
    }

  if (keywords)
    {
      const char *fmt;
      const char *author;

      if (local_mod)
        {
          /* For locally modified files, we'll append an 'M'
             to the revision number, and set the author to
             "(local)" since we can't always determine the
             current user's username */
          fmt = "%ldM";
          author = _("(local)");
        }
      else
        {
          fmt = "%ld";
          author = entry->cmt_author;
        }

      SVN_ERR(svn_subst_build_keywords2
              (&kw, keywords->data,
               apr_psprintf(pool, fmt, entry->cmt_rev),
               entry->url, tm, author, pool));
    }

  /* Our API contract says that OUTPUT will not be closed. The two paths
     below close it, so disown the stream to protect it. The input will
     be closed, which is good (since we opened it). */
  output = svn_stream_disown(output, pool);

  /* Wrap the output stream if translation is needed. */
  if (eol != NULL || kw != NULL)
    output = svn_subst_stream_translated(output, eol, FALSE, kw, TRUE, pool);

  return svn_stream_copy3(input, output, cancel_func, cancel_baton, pool);
}

svn_error_t *
svn_client_cat2(svn_stream_t *out,
                const char *path_or_url,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *revision,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  svn_revnum_t rev;
  svn_node_kind_t url_kind;
  svn_string_t *eol_style;
  svn_string_t *keywords;
  apr_hash_t *props;
  const char *url;
  svn_stream_t *output = out;

  /* ### Inconsistent default revision logic in this command. */
  if (peg_revision->kind == svn_opt_revision_unspecified)
    {
      peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                            path_or_url);
      revision = svn_cl__rev_default_to_head_or_base(revision, path_or_url);
    }
  else
    {
      peg_revision = svn_cl__rev_default_to_head_or_working(peg_revision,
                                                            path_or_url);
      revision = svn_cl__rev_default_to_peg(revision, peg_revision);
    }

  if (! svn_path_is_url(path_or_url)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(peg_revision->kind)
      && SVN_CLIENT__REVKIND_IS_LOCAL_TO_WC(revision->kind))
    {
      svn_wc_adm_access_t *adm_access;

      SVN_ERR(svn_wc_adm_open3(&adm_access, NULL,
                               svn_path_dirname(path_or_url, pool), FALSE,
                               0, ctx->cancel_func, ctx->cancel_baton,
                               pool));

      SVN_ERR(cat_local_file(path_or_url, out, adm_access, revision,
                             ctx->cancel_func, ctx->cancel_baton, pool));

      return svn_wc_adm_close2(adm_access, pool);
    }

  /* Get an RA plugin for this filesystem object. */
  SVN_ERR(svn_client__ra_session_from_path(&ra_session, &rev,
                                           &url, path_or_url, NULL,
                                           peg_revision,
                                           revision, ctx, pool));

  /* Make sure the object isn't a directory. */
  SVN_ERR(svn_ra_check_path(ra_session, "", rev, &url_kind, pool));
  if (url_kind == svn_node_dir)
    return svn_error_createf(SVN_ERR_CLIENT_IS_DIRECTORY, NULL,
                             _("URL '%s' refers to a directory"), url);

  /* Grab some properties we need to know in order to figure out if anything
     special needs to be done with this file. */
  SVN_ERR(svn_ra_get_file(ra_session, "", rev, NULL, NULL, &props, pool));

  eol_style = apr_hash_get(props, SVN_PROP_EOL_STYLE, APR_HASH_KEY_STRING);
  keywords = apr_hash_get(props, SVN_PROP_KEYWORDS, APR_HASH_KEY_STRING);

  if (eol_style || keywords)
    {
      /* It's a file with no special eol style or keywords. */
      svn_subst_eol_style_t eol;
      const char *eol_str;
      apr_hash_t *kw;

      if (eol_style)
        svn_subst_eol_style_from_value(&eol, &eol_str, eol_style->data);
      else
        {
          eol = svn_subst_eol_style_none;
          eol_str = NULL;
        }


      if (keywords)
        {
          svn_string_t *cmt_rev, *cmt_date, *cmt_author;
          apr_time_t when = 0;

          cmt_rev = apr_hash_get(props, SVN_PROP_ENTRY_COMMITTED_REV,
                                 APR_HASH_KEY_STRING);
          cmt_date = apr_hash_get(props, SVN_PROP_ENTRY_COMMITTED_DATE,
                                  APR_HASH_KEY_STRING);
          cmt_author = apr_hash_get(props, SVN_PROP_ENTRY_LAST_AUTHOR,
                                    APR_HASH_KEY_STRING);
          if (cmt_date)
            SVN_ERR(svn_time_from_cstring(&when, cmt_date->data, pool));

          SVN_ERR(svn_subst_build_keywords2
                  (&kw, keywords->data,
                   cmt_rev->data,
                   url,
                   when,
                   cmt_author ? cmt_author->data : NULL,
                   pool));
        }
      else
        kw = NULL;

      /* Interject a translating stream */
      output = svn_subst_stream_translated(svn_stream_disown(out, pool),
                                           eol_str, FALSE, kw, TRUE, pool);
    }

  SVN_ERR(svn_ra_get_file(ra_session, "", rev, output, NULL, NULL, pool));

  if (out != output)
    /* Close the interjected stream */
    SVN_ERR(svn_stream_close(output));

  return SVN_NO_ERROR;
}
