/*
 * constructors.c :  Constructors for various data structures.
 *
 * ====================================================================
 * Copyright (c) 2005-2007 CollabNet.  All rights reserved.
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
#include <apr_strings.h>

#include "svn_types.h"
#include "svn_props.h"
#include "svn_string.h"


svn_commit_info_t *
svn_create_commit_info(apr_pool_t *pool)
{
  svn_commit_info_t *commit_info
    = apr_pcalloc(pool, sizeof(*commit_info));

  commit_info->revision = SVN_INVALID_REVNUM;
  /* All other fields were initialized to NULL above. */

  return commit_info;
}

svn_commit_info_t *
svn_commit_info_dup(const svn_commit_info_t *src_commit_info,
                    apr_pool_t *pool)
{
  svn_commit_info_t *dst_commit_info = svn_create_commit_info(pool);

  dst_commit_info->date = src_commit_info->date
    ? apr_pstrdup(pool, src_commit_info->date) : NULL;
  dst_commit_info->author = src_commit_info->author
    ? apr_pstrdup(pool, src_commit_info->author) : NULL;
  dst_commit_info->revision = src_commit_info->revision;
  dst_commit_info->post_commit_err = src_commit_info->post_commit_err
    ? apr_pstrdup(pool, src_commit_info->post_commit_err) : NULL;

  return dst_commit_info;
}

svn_log_changed_path2_t *
svn_log_changed_path2_create(apr_pool_t *pool)
{
  svn_log_changed_path2_t *new_changed_path
    = apr_pcalloc(pool, sizeof(*new_changed_path));

  return new_changed_path;
}

svn_log_changed_path2_t *
svn_log_changed_path2_dup(const svn_log_changed_path2_t *changed_path,
                          apr_pool_t *pool)
{
  svn_log_changed_path2_t *new_changed_path
    = svn_log_changed_path2_create(pool);

  *new_changed_path = *changed_path;

  if (new_changed_path->copyfrom_path)
    new_changed_path->copyfrom_path =
      apr_pstrdup(pool, new_changed_path->copyfrom_path);

  return new_changed_path;
}

/**
 * Reallocate the members of PROP using POOL.
 */
static void
svn_prop__members_dup(svn_prop_t *prop, apr_pool_t *pool)
{
  if (prop->name)
    prop->name = apr_pstrdup(pool, prop->name);
  if (prop->value)
    prop->value = svn_string_dup(prop->value, pool);
}

svn_prop_t *
svn_prop_dup(const svn_prop_t *prop, apr_pool_t *pool)
{
  svn_prop_t *new_prop = apr_palloc(pool, sizeof(*new_prop));

  *new_prop = *prop;

  svn_prop__members_dup(new_prop, pool);

  return new_prop;
}

apr_array_header_t *
svn_prop_array_dup(const apr_array_header_t *array, apr_pool_t *pool)
{
  int i;
  apr_array_header_t *new_array = apr_array_copy(pool, array);
  for (i = 0; i < new_array->nelts; ++i)
    {
      svn_prop_t *elt = &APR_ARRAY_IDX(new_array, i, svn_prop_t);
      svn_prop__members_dup(elt, pool);
    }
  return new_array;
}

apr_array_header_t *
svn_prop_hash_to_array(apr_hash_t *hash, apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_array_header_t *array = apr_array_make(pool, apr_hash_count(hash),
                                             sizeof(svn_prop_t));

  for (hi = apr_hash_first(pool, hash); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_prop_t prop;

      apr_hash_this(hi, &key, NULL, &val);
      prop.name = key;
      prop.value = val;
      APR_ARRAY_PUSH(array, svn_prop_t) = prop;
    }

  return array;
}

apr_hash_t *
svn_prop_hash_dup(apr_hash_t *hash,
                  apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_hash_t *new_hash;

  new_hash = apr_hash_make(pool);

  for (hi = apr_hash_first(pool, hash); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *prop;

      apr_hash_this(hi, &key, NULL, &prop);

      apr_hash_set(new_hash, apr_pstrdup(pool, key),
      APR_HASH_KEY_STRING, svn_string_dup(prop, pool));
    }
  return new_hash;
}


svn_dirent_t *
svn_dirent_dup(const svn_dirent_t *dirent,
               apr_pool_t *pool)
{
  svn_dirent_t *new_dirent = apr_palloc(pool, sizeof(*new_dirent));

  *new_dirent = *dirent;

  new_dirent->last_author = apr_pstrdup(pool, dirent->last_author);

  return new_dirent;
}

svn_log_entry_t *
svn_log_entry_create(apr_pool_t *pool)
{
  svn_log_entry_t *log_entry = apr_pcalloc(pool, sizeof(*log_entry));

  return log_entry;
}

svn_log_entry_t *
svn_log_entry_dup(svn_log_entry_t *log_entry, apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  svn_log_entry_t *new_entry = svn_log_entry_create(pool);

  *new_entry = *log_entry;

  if (log_entry->revprops)
    log_entry->revprops = svn_prop_hash_dup(log_entry->revprops, pool);

  if (log_entry->changed_paths2)
    {
      new_entry->changed_paths2 = apr_hash_make(pool);

      for (hi = apr_hash_first(pool, log_entry->changed_paths2);
           hi; hi = apr_hash_next(hi))
        {
          const void *key;
          void *change;

          apr_hash_this(hi, &key, NULL, &change);

          apr_hash_set(new_entry->changed_paths2, apr_pstrdup(pool, key),
                       APR_HASH_KEY_STRING,
                       svn_log_changed_path2_dup(change, pool));
        }
    }

  /* We can't copy changed_paths by itself without using deprecated code,
     but we don't have to, as this function was new after the introduction
     of the changed_paths2 field. */
  new_entry->changed_paths = new_entry->changed_paths2;

  return new_entry;
}

svn_location_segment_t *
svn_location_segment_dup(svn_location_segment_t *segment,
                         apr_pool_t *pool)
{
  svn_location_segment_t *new_segment =
    apr_pcalloc(pool, sizeof(*new_segment));
  *new_segment = *segment;
  if (segment->path)
    new_segment->path = apr_pstrdup(pool, segment->path);
  return new_segment;
}
