/*
 * dav_svn.h: types, functions, macros for the DAV/SVN Apache module
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

#ifndef DAV_SVN_H
#define DAV_SVN_H

#include <apr_tables.h>
#include <apr_xml.h>

#include <httpd.h>
#include <http_log.h>
#include <mod_dav.h>

#include "svn_error.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_path.h"
#include "svn_xml.h"
#include "private/svn_dav_protocol.h"
#include "mod_authz_svn.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* what the one VCC is called */
#define DAV_SVN__DEFAULT_VCC_NAME        "default"

/* a pool-key for the shared dav_svn_root used by autoversioning  */
#define DAV_SVN__AUTOVERSIONING_ACTIVITY "svn-autoversioning-activity"


/* dav_svn_repos
 *
 * Record information about the repository that a resource belongs to.
 * This structure will be shared between multiple resources so that we
 * can optimized our FS access.
 *
 * Note that we do not refcount this structure. Presumably, we will need
 * it throughout the life of the request. Therefore, we can just leave it
 * for the request pool to cleanup/close.
 *
 * Also, note that it is possible that two resources may have distinct
 * dav_svn_repos structures, yet refer to the same repository. This is
 * allowed by the SVN FS interface.
 *
 * ### should we attempt to merge them when we detect this situation in
 * ### places like is_same_resource, is_parent_resource, or copy/move?
 * ### I say yes: the FS will certainly have an easier time if there is
 * ### only a single FS open; otherwise, it will have to work a bit harder
 * ### to keep the things in sync.
 */
typedef struct {
  apr_pool_t *pool;     /* request_rec -> pool */

  /* Remember the root URL path of this repository (just a path; no
     scheme, host, or port).

     Example: the URI is "http://host/repos/file", this will be "/repos".

     This always starts with "/", and if there are any components
     beyond that, then it does not end with "/".
  */
  const char *root_path;

  /* Remember an absolute URL for constructing other URLs. In the above
     example, this would be "http://host" (note: no trailing slash)
  */
  const char *base_url;

  /* Remember the special URI component for this repository */
  const char *special_uri;

  /* This records the filesystem path to the SVN FS */
  const char *fs_path;

  /* The name of this repository */
  const char *repo_name;

  /* The repository filesystem basename */
  const char *repo_basename;

  /* The URI of the XSL transform for directory indexes */
  const char *xslt_uri;

  /* Whether autoversioning is active for this repository. */
  svn_boolean_t autoversioning;

  /* Whether bulk updates are allowed for this repository. */
  svn_boolean_t bulk_updates;

  /* the open repository */
  svn_repos_t *repos;

  /* a cached copy of REPOS->fs above. */
  svn_fs_t *fs;

  /* the user operating against this repository */
  const char *username;

  /* is the client a Subversion client? */
  svn_boolean_t is_svn_client;

  /* The client's capabilities.  Maps SVN_RA_CAPABILITY_* keys to
     "yes" or "no" values.  If a capability is not yet discovered, it
     is absent from the table.  The table itself is allocated in this
     structure's 'pool' field, and the keys and values must have at
     least that lifetime.  Most likely the keys and values are
     constants anyway (and sufficiently well-informed internal code
     may therefore compare against those constants' addresses).  If
     'is_svn_client' is false, then 'capabilities' should be empty. */
  apr_hash_t *client_capabilities;

  /* The path to the activities db */
  const char *activities_db;

} dav_svn_repos;


/*
** dav_svn_private_restype: identifiers for our different private resources
**
** There are some resources within mod_dav_svn that are "privately defined".
** This isn't so much to prevent other people from knowing what they are,
** but merely that mod_dav doesn't have a standard name for them.
*/
enum dav_svn_private_restype {
  DAV_SVN_RESTYPE_UNSET,

  DAV_SVN_RESTYPE_ROOT_COLLECTION,      /* .../!svn/     */
  DAV_SVN_RESTYPE_VER_COLLECTION,       /* .../!svn/ver/ */
  DAV_SVN_RESTYPE_HIS_COLLECTION,       /* .../!svn/his/ */
  DAV_SVN_RESTYPE_WRK_COLLECTION,       /* .../!svn/wrk/ */
  DAV_SVN_RESTYPE_ACT_COLLECTION,       /* .../!svn/act/ */
  DAV_SVN_RESTYPE_VCC_COLLECTION,       /* .../!svn/vcc/ */
  DAV_SVN_RESTYPE_BC_COLLECTION,        /* .../!svn/bc/  */
  DAV_SVN_RESTYPE_BLN_COLLECTION,       /* .../!svn/bln/ */
  DAV_SVN_RESTYPE_WBL_COLLECTION,       /* .../!svn/wbl/ */
  DAV_SVN_RESTYPE_VCC,                  /* .../!svn/vcc/NAME */
  DAV_SVN_RESTYPE_PARENTPATH_COLLECTION /* see SVNParentPath directive */
};


/* store info about a root in a repository */
typedef struct {
  /* If a root within the FS has been opened, the value is stored here.
     Otherwise, this field is NULL. */
  svn_fs_root_t *root;

  /* If the root has been opened, and it was opened for a specific revision,
     then it is contained in REV. If the root is unopened or corresponds to
     a transaction, then REV will be SVN_INVALID_REVNUM. */
  svn_revnum_t rev;

  /* If this resource is an activity or part of an activity, this specifies
     the ID of that activity. It may not (yet) correspond to a transaction
     in the FS.

     WORKING and ACTIVITY resources use this field.
  */
  const char *activity_id;

  /* If the root is part of a transaction, this contains the FS's tranaction
     name. It may be NULL if this root corresponds to a specific revision.
     It may also be NULL if we have not opened the root yet.

     WORKING and ACTIVITY resources use this field.
  */
  const char *txn_name;

  /* If the root is part of a transaction, this contains the FS's transaction
     handle. It may be NULL if this root corresponds to a specific revision.
     It may also be NULL if we have not opened the transaction yet.

     WORKING resources use this field.
  */
  svn_fs_txn_t *txn;

} dav_svn_root;


/* internal structure to hold information about this resource */
struct dav_resource_private {
  /* Path from the SVN repository root to this resource. This value has
     a leading slash. It will never have a trailing slash, even if the
     resource represents a collection.

     For example: URI is http://host/repos/file -- path will be "/file".

     NOTE: this path is from the URI and does NOT necessarily correspond
           to a path within the FS repository.
  */
  svn_stringbuf_t *uri_path;

  /* The FS repository path to this resource, with a leading "/". Note
     that this is "/" the root. This value will be NULL for resources
     that have no corresponding resource within the repository (such as
     the PRIVATE resources, Baselines, or Working Baselines). */
  const char *repos_path;

  /* the FS repository this resource is associated with */
  dav_svn_repos *repos;

  /* what FS root this resource occurs within */
  dav_svn_root root;

  /* for PRIVATE resources: the private resource type */
  enum dav_svn_private_restype restype;

  /* The request which created this resource.  We need this to
     generate subrequests. */
  request_rec *r;

  /* ### hack to deal with the Content-Type header on a PUT */
  int is_svndiff;

  /* ### record the base for computing a delta during a GET */
  const char *delta_base;

  /* SVNDIFF version we can transmit to the client.  */
  int svndiff_version;

  /* the value of any SVN_DAV_OPTIONS_HEADER that came in the request */
  const char *svn_client_options;

  /* the revnum value from a possible SVN_DAV_VERSION_NAME_HEADER */
  svn_revnum_t version_name;

  /* Hex MD5 digests for base text and resultant fulltext.
     Either or both of these may be null, in which case ignored. */
  const char *base_checksum;
  const char *result_checksum;

  /* was this resource auto-checked-out? */
  svn_boolean_t auto_checked_out;

  /* Pool to allocate temporary data from */
  apr_pool_t *pool;
};


/* Every provider needs to define an opaque locktoken type. */
struct dav_locktoken
{
  /* This is identical to the 'token' field of an svn_lock_t. */
  const char *uuid_str;
};


/* for the repository referred to by this request, where is the SVN FS? */
const char *dav_svn__get_fs_path(request_rec *r);
const char *dav_svn__get_fs_parent_path(request_rec *r);

/* for the repository referred to by this request, is autoversioning active? */
svn_boolean_t dav_svn__get_autoversioning_flag(request_rec *r);

/* for the repository referred to by this request, are bulk updates allowed? */
svn_boolean_t dav_svn__get_bulk_updates_flag(request_rec *r);

/* for the repository referred to by this request, are subrequests active? */
svn_boolean_t dav_svn__get_pathauthz_flag(request_rec *r);

/* for the repository referred to by this request, are subrequests bypassed?
 * A function pointer if yes, NULL if not.
 */
authz_svn__subreq_bypass_func_t dav_svn__get_pathauthz_bypass(request_rec *r);

/* for the repository referred to by this request, is a GET of
   SVNParentPath allowed? */
svn_boolean_t dav_svn__get_list_parentpath_flag(request_rec *r);


/* SPECIAL URI

   SVN needs to create many types of "pseudo resources" -- resources
   that don't correspond to the users' files/directories in the
   repository. Specifically, these are:

   - working resources
   - activities
   - version resources
   - version history resources

   Each of these will be placed under a portion of the URL namespace
   that defines the SVN repository. For example, let's say the user
   has configured an SVN repository at http://host/svn/repos. The
   special resources could be configured to live at .../!svn/ under
   that repository. Thus, an activity might be located at
   http://host/svn/repos/!svn/act/1234.

   The special URI is configurable on a per-server basis and defaults
   to "!svn".

   NOTE: the special URI is RELATIVE to the "root" of the
   repository. The root is generally available only to
   dav_svn_get_resource(). This is okay, however, because we can cache
   the root_dir when the resource structure is built.
*/

/* Return the special URI to be used for this resource. */
const char *dav_svn__get_special_uri(request_rec *r);

/* Return a descriptive name for the repository */
const char *dav_svn__get_repo_name(request_rec *r);

/* Return the URI of an XSL transform stylesheet */
const char *dav_svn__get_xslt_uri(request_rec *r);

/* Return the master URI (for mirroring) */
const char * dav_svn__get_master_uri(request_rec *r);

/* Return the activities db */
const char * dav_svn__get_activities_db(request_rec *r);

/* Return the root directory */
const char * dav_svn__get_root_dir(request_rec *r);

/*** activity.c ***/

/* activity functions for looking up, storing, and deleting
   ACTIVITY->TXN mappings */
const char *
dav_svn__get_txn(const dav_svn_repos *repos, const char *activity_id);

dav_error *
dav_svn__delete_activity(const dav_svn_repos *repos, const char *activity_id);

dav_error *
dav_svn__store_activity(const dav_svn_repos *repos,
                        const char *activity_id,
                        const char *txn_name);

dav_error *
dav_svn__create_activity(const dav_svn_repos *repos,
                         const char **ptxn_name,
                         apr_pool_t *pool);


/*** repos.c ***/

/* generate an ETag for RESOURCE and return it, allocated in POOL. */
const char *
dav_svn__getetag(const dav_resource *resource, apr_pool_t *pool);

/*
  Construct a working resource for a given resource.

  The internal information (repository, URL parts, etc) for the new
  resource comes from BASE, the activity to use is specified by
  ACTIVITY_ID, and the name of the transaction is specified by
  TXN_NAME. These will be assembled into a new dav_resource and
  returned.

  If TWEAK_IN_PLACE is non-zero, then directly tweak BASE into a
  working resource and return NULL.
*/
dav_resource *
dav_svn__create_working_resource(dav_resource *base,
                                 const char *activity_id,
                                 const char *txn_name,
                                 int tweak_in_place);
/*
   Convert a working RESOURCE back into a regular one, in-place.

   In particular: change the resource type to regular, removing the
   'working' flag, change the fs root from a txn-root to a rev-root,
   and set the URL back into either a public URL or bc URL.
*/
dav_error *
dav_svn__working_to_regular_resource(dav_resource *resource);

/*
   Given a version-resource URI, construct a new version-resource in
   POOL and return it in  *VERSION_RES.
*/
dav_error *
dav_svn__create_version_resource(dav_resource **version_res,
                                 const char *uri,
                                 apr_pool_t *pool);

extern const dav_hooks_repository dav_svn__hooks_repository;


/*** deadprops.c ***/
extern const dav_hooks_propdb dav_svn__hooks_propdb;


/*** lock.c ***/
extern const dav_hooks_locks dav_svn__hooks_locks;


/*** version.c ***/

/* For an autoversioning commit, a helper function which attaches an
   auto-generated 'svn:log' property to a txn, as well as a property
   that indicates the revision was made via autoversioning. */
svn_error_t *
dav_svn__attach_auto_revprops(svn_fs_txn_t *txn,
                              const char *fs_path,
                              apr_pool_t *pool);


/* Hook function of types 'checkout' and 'checkin', as defined in
   mod_dav.h's versioning provider hooks table (see dav_hooks_vsn).  */
dav_error *
dav_svn__checkout(dav_resource *resource,
                  int auto_checkout,
                  int is_unreserved,
                  int is_fork_ok,
                  int create_activity,
                  apr_array_header_t *activities,
                  dav_resource **working_resource);

dav_error *
dav_svn__checkin(dav_resource *resource,
                 int keep_checked_out,
                 dav_resource **version_resource);


/* Helper for reading lock-tokens out of request bodies, by looking
   for cached body in R->pool's userdata.

   Return a hash that maps (const char *) absolute fs paths to (const
   char *) locktokens.  Allocate the hash and all keys/vals in POOL.
   PATH_PREFIX is the prefix we need to prepend to each relative
   'lock-path' in the xml in order to create an absolute fs-path.  */
dav_error *
dav_svn__build_lock_hash(apr_hash_t **locks,
                         request_rec *r,
                         const char *path_prefix,
                         apr_pool_t *pool);


/* Helper: push all of the lock-tokens (hash values) in LOCKS into
   RESOURCE's already-open svn_fs_t. */
dav_error *
dav_svn__push_locks(dav_resource *resource,
                    apr_hash_t *locks,
                    apr_pool_t *pool);


extern const dav_hooks_vsn dav_svn__hooks_vsn;


/*** liveprops.c ***/

extern const dav_liveprop_group dav_svn__liveprop_group;

/*
  LIVE PROPERTY HOOKS

  These are standard hooks defined by mod_dav. We implement them to expose
  various live properties on the resources under our control.

  gather_propsets: appends URIs into the array; the property set URIs are
                   used to specify which sets of custom properties we
                   define/expose.
  find_liveprop: given a namespace and name, return the hooks for the
                 provider who defines that property.
  insert_all_liveprops: for a given resource, insert all of the live
                        properties defined on that resource. The properties
                        are inserted according to the WHAT parameter.
*/
void dav_svn__gather_propsets(apr_array_header_t *uris);

int
dav_svn__find_liveprop(const dav_resource *resource,
                       const char *ns_uri,
                       const char *name,
                       const dav_hooks_liveprop **hooks);

void
dav_svn__insert_all_liveprops(request_rec *r,
                              const dav_resource *resource,
                              dav_prop_insert what,
                              apr_text_header *phdr);


/*** merge.c ***/

/* Generate the HTTP response body for a successful MERGE. */
/* ### more docco */
dav_error *
dav_svn__merge_response(ap_filter_t *output,
                        const dav_svn_repos *repos,
                        svn_revnum_t new_rev,
                        char *post_commit_err,
                        apr_xml_elem *prop_elem,
                        svn_boolean_t disable_merge_response,
                        apr_pool_t *pool);


/*** reports/ ***/

/* The list of Subversion's custom REPORTs. */
/* ### should move these report names to a public header to share with
   ### the client (and third parties). */
static const dav_report_elem dav_svn__reports_list[] = {
  { SVN_XML_NAMESPACE, "update-report" },
  { SVN_XML_NAMESPACE, "log-report" },
  { SVN_XML_NAMESPACE, "dated-rev-report" },
  { SVN_XML_NAMESPACE, "get-locations" },
  { SVN_XML_NAMESPACE, "get-location-segments" },
  { SVN_XML_NAMESPACE, "file-revs-report" },
  { SVN_XML_NAMESPACE, "get-locks-report" },
  { SVN_XML_NAMESPACE, "replay-report" },
  { SVN_XML_NAMESPACE, "get-deleted-rev-report" },
  { SVN_XML_NAMESPACE, SVN_DAV__MERGEINFO_REPORT },
  { NULL, NULL },
};


/* The various report handlers, defined in reports/, and used by version.c.  */
dav_error *
dav_svn__update_report(const dav_resource *resource,
                       const apr_xml_doc *doc,
                       ap_filter_t *output);
dav_error *
dav_svn__log_report(const dav_resource *resource,
                    const apr_xml_doc *doc,
                    ap_filter_t *output);
dav_error *
dav_svn__dated_rev_report(const dav_resource *resource,
                          const apr_xml_doc *doc,
                          ap_filter_t *output);
dav_error *
dav_svn__get_locations_report(const dav_resource *resource,
                              const apr_xml_doc *doc,
                              ap_filter_t *output);
dav_error *
dav_svn__get_location_segments_report(const dav_resource *resource,
                                      const apr_xml_doc *doc,
                                      ap_filter_t *output);
dav_error *
dav_svn__file_revs_report(const dav_resource *resource,
                          const apr_xml_doc *doc,
                          ap_filter_t *output);
dav_error *
dav_svn__replay_report(const dav_resource *resource,
                       const apr_xml_doc *doc,
                       ap_filter_t *output);
dav_error *
dav_svn__get_mergeinfo_report(const dav_resource *resource,
                              const apr_xml_doc *doc,
                              ap_filter_t *output);
dav_error *
dav_svn__get_locks_report(const dav_resource *resource,
                          const apr_xml_doc *doc,
                          ap_filter_t *output);

dav_error *
dav_svn__get_deleted_rev_report(const dav_resource *resource,
                                const apr_xml_doc *doc,
                                ap_filter_t *output);

/*** authz.c ***/

/* A baton needed by dav_svn__authz_read_func(). */
typedef struct
{
  /* The original request, needed to generate a subrequest. */
  request_rec *r;

  /* We need this to construct a URI based on a repository abs path. */
  const dav_svn_repos *repos;

} dav_svn__authz_read_baton;


/* Convert incoming RESOURCE and revision REV into a version-resource URI and
   perform a GET subrequest on it.  This will invoke any authz modules loaded
   into apache. Return TRUE if the subrequest succeeds, FALSE otherwise.

   If REV is SVN_INVALID_REVNUM, then we look at HEAD.
   Use POOL for any temporary allocation.
*/
svn_boolean_t
dav_svn__allow_read(const dav_resource *resource,
                   svn_revnum_t rev,
                   apr_pool_t *pool);

/* If authz is enabled in the specified BATON, return a read authorization
   function. Otherwise, return NULL. */
svn_repos_authz_func_t
dav_svn__authz_read_func(dav_svn__authz_read_baton *baton);


/*** util.c ***/

/* A wrapper around mod_dav's dav_new_error_tag, mod_dav_svn uses this
   instead of the mod_dav function to enable special mod_dav_svn specific
   processing.  See dav_new_error_tag for parameter documentation.
   Note that DESC may be null (it's hard to track this down from
   dav_new_error_tag()'s documentation, but see the dav_error type,
   which says that its desc field may be NULL). */
dav_error *
dav_svn__new_error_tag(apr_pool_t *pool,
                       int status,
                       int errno_id,
                       const char *desc,
                       const char *namespace,
                       const char *tagname);


/* Convert an svn_error_t into a dav_error, pushing another error based on
   MESSAGE if MESSAGE is not NULL.  Use the provided HTTP status for the
   DAV errors.  Allocate new DAV errors from POOL.

   NOTE: this function destroys (cleanly, of course) SERR after it has
   copied/converted its data to the new DAV error.

   NOTE: MESSAGE needs to hang around for the lifetime of the error since
   the current implementation doesn't copy it!  Lots of callers pass static
   string constant. */
dav_error *
dav_svn__convert_err(svn_error_t *serr,
                    int status,
                    const char *message,
                    apr_pool_t *pool);


/* Compare (PATH in ROOT) to (PATH in ROOT/PATH's created_rev).

   If these nodes are identical, then return the created_rev.

   If the nodes aren't identical, or if PATH simply doesn't exist in
   the created_rev, then return the revision represented by ROOT.

   (This is a helper to functions that want to build version-urls and
    use the CR if possible.) */
svn_revnum_t
dav_svn__get_safe_cr(svn_fs_root_t *root, const char *path, apr_pool_t *pool);


/* Construct various kinds of URIs.

   REPOS is always required, as all URIs will be built to refer to elements
   within that repository. WHAT specifies the type of URI to build. The
   ADD_HREF flag determines whether the URI is to be wrapped inside of
   <D:href>uri</D:href> elements (for inclusion in a response).

   Different pieces of information are required for the various URI types:

   ACT_COLLECTION: no additional params required
   BASELINE:       REVISION should be specified
   BC:             REVISION should be specified
   PUBLIC:         PATH should be specified with a leading slash
   VERSION:        REVISION and PATH should be specified
   VCC:            no additional params required
*/
enum dav_svn__build_what {
  DAV_SVN__BUILD_URI_ACT_COLLECTION, /* the collection of activities */
  DAV_SVN__BUILD_URI_BASELINE,   /* a Baseline */
  DAV_SVN__BUILD_URI_BC,         /* a Baseline Collection */
  DAV_SVN__BUILD_URI_PUBLIC,     /* the "public" VCR */
  DAV_SVN__BUILD_URI_VERSION,    /* a Version Resource */
  DAV_SVN__BUILD_URI_VCC         /* a Version Controlled Configuration */
};

const char *
dav_svn__build_uri(const dav_svn_repos *repos,
                   enum dav_svn__build_what what,
                   svn_revnum_t revision,
                   const char *path,
                   int add_href,
                   apr_pool_t *pool);


/* Simple parsing of a URI. This is used for URIs which appear within a
   request body. It enables us to verify and break out the necessary pieces
   to figure out what is being referred to.

   ### this is horribly duplicative with the parsing functions in repos.c
   ### for now, this implements only a minor subset of the full range of
   ### URIs which we may need to parse. it also ignores any scheme, host,
   ### and port in the URI and simply assumes it refers to the same server.
*/
typedef struct {
  svn_revnum_t rev;
  const char *repos_path;
  const char *activity_id;
} dav_svn__uri_info;

svn_error_t *
dav_svn__simple_parse_uri(dav_svn__uri_info *info,
                          const dav_resource *relative,
                          const char *uri,
                          apr_pool_t *pool);


int dav_svn__find_ns(apr_array_header_t *namespaces, const char *uri);


/* Output XML data to OUTPUT using BB.  Use FMT as format string for the
   output. */
svn_error_t *
dav_svn__send_xml(apr_bucket_brigade *bb,
                  ap_filter_t *output,
                  const char *fmt,
                  ...)
       __attribute__((format(printf, 3, 4)));


/* Test PATH for canonicalness (defined as "what won't make the
   svn_path_* functions immediately explode"), returning an
   HTTP_BAD_REQUEST error tag if the test fails. */
dav_error *dav_svn__test_canonical(const char *path, apr_pool_t *pool);


/* Convert @a serr into a dav_error.  If @a new_msg is non-NULL, use
   @a new_msg in the returned error, and write the original
   @a serr->message to httpd's log.  Destroy the passed-in @a serr,
   similarly to dav_svn__convert_err().

   @a new_msg is usually a "sanitized" version of @a serr->message.
   That is, if @a serr->message contains security-sensitive data,
   @a new_msg does not.

   The purpose of sanitization is to prevent security-sensitive data
   from being transmitted over the network to the client.  The error
   messages produced by various APIs (e.g., svn_fs, svn_repos) may
   contain security-sensitive data such as the actual server file
   system's path to the repository.  We don't want to send that to the
   client, but we do want to log the real error on the server side.
 */
dav_error *
dav_svn__sanitize_error(svn_error_t *serr,
                        const char *new_msg,
                        int http_status,
                        request_rec *r);


/* Return a writable generic stream that will encode its output to base64
   and send it to the Apache filter OUTPUT using BB.  Allocate the stream in
   POOL. */
svn_stream_t *
dav_svn__make_base64_output_stream(apr_bucket_brigade *bb,
                                   ap_filter_t *output,
                                   apr_pool_t *pool);

/* In INFO->r->subprocess_env set "SVN-ACTION" to LINE, "SVN-REPOS" to
 * INFO->repos->fs_path, and "SVN-REPOS-NAME" to INFO->repos->repo_basename. */
void
dav_svn__operational_log(struct dav_resource_private *info, const char *line);

/* Flush BB if it's okay and useful to do so, but treat PREFERRED_ERR
 * as a more important error to return (if it is non-NULL).
 *
 * This is intended to be used at the end of response processing,
 * probably called as a direct return generator, like so:
 *
 *   return dav_svn__final_flush_or_error(r, bb, output, derr, resource->pool);
 *
 * SOME BACKGROUND INFO:
 *
 * We don't flush the brigade unless there's something in it to
 * flush; that way, we have the opportunity to package a dav_error up
 * for transmission back to the client.
 *
 * To understand this, see mod_dav.c:dav_method_report(): as long as
 * it doesn't think we've sent anything to the client, it'll send
 * the real error, which is what we'd prefer.  This situation is
 * described in httpd-2.2.6/modules/dav/main/mod_dav.c, line 4066,
 * in the comment in dav_method_report() that says:
 *
 *    If an error occurred during the report delivery, there's
 *    basically nothing we can do but abort the connection and
 *    log an error.  This is one of the limitations of HTTP; it
 *    needs to "know" the entire status of the response before
 *    generating it, which is just impossible in these streamy
 *    response situations.
 *
 * In other words, flushing the brigade causes r->sent_bodyct (see
 * dav_method_report()) to become non-zero, even if we hadn't tried to
 * send any data to the brigade yet.  So we don't flush unless data
 * was actually sent.
 */
dav_error *
dav_svn__final_flush_or_error(request_rec *r, apr_bucket_brigade *bb,
                              ap_filter_t *output, dav_error *preferred_err,
                              apr_pool_t *pool);

/*** mirror.c ***/

/* Perform the fixup hook for the R request.  */
int dav_svn__proxy_merge_fixup(request_rec *r);

/* An Apache input filter which rewrites the locations in headers and
   request body.  It reads from filter F using BB data, MODE mode, BLOCK
   blocking strategy, and READBYTES. */
apr_status_t dav_svn__location_in_filter(ap_filter_t *f,
                                         apr_bucket_brigade *bb,
                                         ap_input_mode_t mode,
                                         apr_read_type_e block,
                                         apr_off_t readbytes);

/* An Apache output filter F which rewrites the response headers for
 * location headers.  It will modify the stream in BB. */
apr_status_t dav_svn__location_header_filter(ap_filter_t *f,
                                             apr_bucket_brigade *bb);

/* An Apache output filter F which rewrites the response body for
 * location headers.  It will modify the stream in BB. */
apr_status_t dav_svn__location_body_filter(ap_filter_t *f,
                                           apr_bucket_brigade *bb);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAV_SVN_H */
