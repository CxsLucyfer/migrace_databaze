/* txn-table.h : internal interface to ops on `transactions' table
 *
 * ====================================================================
 * Copyright (c) 2000-2004 CollabNet.  All rights reserved.
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

#ifndef SVN_LIBSVN_FS_TXN_TABLE_H
#define SVN_LIBSVN_FS_TXN_TABLE_H

#include "svn_fs.h"
#include "svn_error.h"
#include "../trail.h"
#include "../fs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Open a `transactions' table in ENV.  If CREATE is non-zero, create
   one if it doesn't exist.  Set *TRANSACTIONS_P to the new table.
   Return a Berkeley DB error code.  */
int svn_fs_bdb__open_transactions_table(DB **transactions_p,
                                        DB_ENV *env,
                                        svn_boolean_t create);


/* Create a new transaction in FS as part of TRAIL, with an initial
   root and base root ID of ROOT_ID.  Set *TXN_NAME_P to the name of the
   new transaction, allocated in POOL.  */
svn_error_t *svn_fs_bdb__create_txn(const char **txn_name_p,
                                    svn_fs_t *fs,
                                    const svn_fs_id_t *root_id,
                                    trail_t *trail,
                                    apr_pool_t *pool);


/* Remove the transaction whose name is TXN_NAME from the `transactions'
   table of FS, as part of TRAIL.

   Returns SVN_ERR_FS_TRANSACTION_NOT_MUTABLE if TXN_NAME refers to a
   transaction that has already been committed.  */
svn_error_t *svn_fs_bdb__delete_txn(svn_fs_t *fs,
                                    const char *txn_name,
                                    trail_t *trail,
                                    apr_pool_t *pool);


/* Retrieve the transaction *TXN_P for the Subversion transaction
   named TXN_NAME from the `transactions' table of FS, as part of
   TRAIL.  Perform all allocations in POOL.

   If there is no such transaction, SVN_ERR_FS_NO_SUCH_TRANSACTION is
   the error returned.  */
svn_error_t *svn_fs_bdb__get_txn(transaction_t **txn_p,
                                 svn_fs_t *fs,
                                 const char *txn_name,
                                 trail_t *trail,
                                 apr_pool_t *pool);


/* Store the Suversion transaction TXN in FS with an ID of TXN_NAME as
   part of TRAIL. */
svn_error_t *svn_fs_bdb__put_txn(svn_fs_t *fs,
                                 const transaction_t *txn,
                                 const char *txn_name,
                                 trail_t *trail,
                                 apr_pool_t *pool);


/* Set *NAMES_P to an array of const char * IDs (unfinished
   transactions in FS) as part of TRAIL.  Allocate the array and the
   names in POOL, and use POOL for any temporary allocations.  */
svn_error_t *svn_fs_bdb__get_txn_list(apr_array_header_t **names_p,
                                      svn_fs_t *fs,
                                      trail_t *trail,
                                      apr_pool_t *pool);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_LIBSVN_FS_TXN_TABLE_H */
