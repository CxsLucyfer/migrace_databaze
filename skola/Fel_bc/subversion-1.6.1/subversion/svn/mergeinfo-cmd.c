/*
 * mergeinfo-cmd.c -- Query merge-relative info.
 *
 * ====================================================================
 * Copyright (c) 2007 CollabNet.  All rights reserved.
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

#include "svn_pools.h"
#include "svn_client.h"
#include "svn_cmdline.h"
#include "svn_path.h"
#include "svn_error.h"
#include "svn_error_codes.h"
#include "svn_types.h"
#include "cl.h"

#include "svn_private_config.h"


/*** Code. ***/

/* Implements the svn_log_entry_receiver_t interface.  BATON is a
   pointer to a mergeinfo rangelist array. */
static svn_error_t *
print_log_rev(void *baton,
              svn_log_entry_t *log_entry,
              apr_pool_t *pool)
{
  svn_cmdline_printf(pool, "r%ld\n", log_entry->revision);
  return SVN_NO_ERROR;
}


/* This implements the `svn_opt_subcommand_t' interface. */
svn_error_t *
svn_cl__mergeinfo(apr_getopt_t *os,
                  void *baton,
                  apr_pool_t *pool)
{
  svn_cl__opt_state_t *opt_state = ((svn_cl__cmd_baton_t *) baton)->opt_state;
  svn_client_ctx_t *ctx = ((svn_cl__cmd_baton_t *) baton)->ctx;
  apr_array_header_t *targets;
  const char *source, *target;
  svn_opt_revision_t src_peg_revision, tgt_peg_revision;

  SVN_ERR(svn_cl__args_to_target_array_print_reserved(&targets, os,
                                                      opt_state->targets,
                                                      ctx, pool));

  /* We expect a single source URL followed by a single target --
     nothing more, nothing less. */
  if (targets->nelts < 1)
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Not enough arguments given"));
  if (targets->nelts > 2)
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Too many arguments given"));

  /* Parse the SOURCE-URL[@REV] argument. */
  SVN_ERR(svn_opt_parse_path(&src_peg_revision, &source,
                             APR_ARRAY_IDX(targets, 0, const char *), pool));

  /* Parse the TARGET[@REV] argument (if provided). */
  if (targets->nelts == 2)
    {
      SVN_ERR(svn_opt_parse_path(&tgt_peg_revision, &target,
                                 APR_ARRAY_IDX(targets, 1, const char *),
                                 pool));
    }
  else
    {
      target = "";
      tgt_peg_revision.kind = svn_opt_revision_unspecified;
    }

  /* If no peg-rev was attached to the source URL, assume HEAD. */
  if (src_peg_revision.kind == svn_opt_revision_unspecified)
    src_peg_revision.kind = svn_opt_revision_head;

  /* If no peg-rev was attached to a URL target, then assume HEAD; if
     no peg-rev was attached to a non-URL target, then assume BASE. */
  if (tgt_peg_revision.kind == svn_opt_revision_unspecified)
    {
      if (svn_path_is_url(target))
        tgt_peg_revision.kind = svn_opt_revision_head;
      else
        tgt_peg_revision.kind = svn_opt_revision_base;
    }

  /* Do the real work, depending on the requested data flavor. */
  if (opt_state->show_revs == svn_cl__show_revs_merged)
    {
      SVN_ERR(svn_client_mergeinfo_log_merged(target, &tgt_peg_revision,
                                              source, &src_peg_revision,
                                              print_log_rev, NULL,
                                              FALSE, NULL, ctx, pool));
    }
  else if (opt_state->show_revs == svn_cl__show_revs_eligible)
    {
      SVN_ERR(svn_client_mergeinfo_log_eligible(target, &tgt_peg_revision,
                                                source, &src_peg_revision,
                                                print_log_rev, NULL,
                                                FALSE, NULL, ctx, pool));
    }
  return SVN_NO_ERROR;
}
