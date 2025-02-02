/* dbt.h --- interface to DBT-frobbing functions
 *
 * ====================================================================
 * Copyright (c) 2000-2004, 2009 CollabNet.  All rights reserved.
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

#ifndef SVN_LIBSVN_FS_DBT_H
#define SVN_LIBSVN_FS_DBT_H

#include <apr_pools.h>

#define SVN_WANT_BDB
#include "svn_private_config.h"

#include "svn_fs.h"
#include "private/svn_skel.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Set all fields of DBT to zero.  Return DBT.  */
DBT *svn_fs_base__clear_dbt(DBT *dbt);


/* Set DBT to retrieve no data.  This is useful when you're just
   probing the table to see if an entry exists, or to find a key, but
   don't care what the value is.  Return DBT.  */
DBT *svn_fs_base__nodata_dbt(DBT *dbt);


/* Set DBT to refer to the SIZE bytes at DATA.  Return DBT.  */
DBT *svn_fs_base__set_dbt(DBT *dbt, const void *data, u_int32_t size);


/* Prepare DBT to hold data returned from Berkeley DB.  Return DBT.

   Clear all its fields to zero, but set the DB_DBT_MALLOC flag,
   requesting that Berkeley DB place the returned data in a freshly
   malloc'd block.  If the database operation succeeds, the caller
   then owns the data block, and is responsible for making sure it
   gets freed.

   You can use this with svn_fs_base__track_dbt:

       svn_fs_base__result_dbt (&foo);
       ... some Berkeley DB operation that puts data in foo ...
       svn_fs_base__track_dbt (&foo, pool);

   This arrangement is:
   - thread-safe --- the returned data is allocated via malloc, and
     won't be overwritten if some other thread performs an operation
     on the same table.  See the explanation of ``Retrieved key/data
     permanence'' in the section of the Berkeley DB manual on the DBT
     type.
   - pool-friendly --- the data returned by Berkeley DB is now guaranteed
     to be freed when POOL is cleared.  */
DBT *svn_fs_base__result_dbt(DBT *dbt);

/* Arrange for POOL to `track' DBT's data: when POOL is cleared,
   DBT->data will be freed, using `free'.  If DBT->data is zero,
   do nothing.

   This is meant for use with svn_fs_base__result_dbt; see the explanation
   there.  */
DBT *svn_fs_base__track_dbt(DBT *dbt, apr_pool_t *pool);


/* Prepare DBT for use as a key into a RECNO table.  This call makes
   DBT refer to the db_recno_t pointed to by RECNO as its buffer; the
   record number you assign to *RECNO will be the table key.  */
DBT *svn_fs_base__recno_dbt(DBT *dbt, db_recno_t *recno);


/* Compare two DBT values in byte-by-byte lexicographic order.  */
int svn_fs_base__compare_dbt(const DBT *a, const DBT *b);


/* Set DBT to the unparsed form of ID; allocate memory from POOL.
   Return DBT.  */
DBT *svn_fs_base__id_to_dbt(DBT *dbt, const svn_fs_id_t *id,
                            apr_pool_t *pool);


/* Set DBT to the unparsed form of SKEL; allocate memory from POOL.
   Return DBT.  */
DBT *svn_fs_base__skel_to_dbt(DBT *dbt, svn_skel_t *skel, apr_pool_t *pool);


/* Set DBT to the text of the null-terminated string STR.  DBT will
   refer to STR's storage.  Return DBT.  */
DBT *svn_fs_base__str_to_dbt(DBT *dbt, const char *str);


/* Set DBT to the bytes contained by CHECKSUM.   DBT will refer to CHECKSUM's
   storage.  Return DBT.*/
DBT *svn_fs_base__checksum_to_dbt(DBT* dbt, svn_checksum_t *checksum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_LIBSVN_FS_DBT_H */
