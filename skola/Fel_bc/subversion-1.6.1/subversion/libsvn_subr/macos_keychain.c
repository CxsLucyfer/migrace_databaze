/*
 * macos_keychain.c: Mac OS keychain providers for SVN_AUTH_*
 *
 * ====================================================================
 * Copyright (c) 2003-2006, 2008 CollabNet.  All rights reserved.
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

#include <apr_pools.h>
#include "svn_auth.h"
#include "svn_error.h"
#include "svn_utf.h"
#include "svn_config.h"
#include "svn_user.h"

#include "private/svn_auth_private.h"

#include "svn_private_config.h"

#ifdef SVN_HAVE_KEYCHAIN_SERVICES

#include <Security/Security.h>

/*-----------------------------------------------------------------------*/
/* keychain simple provider, puts passwords in the KeyChain              */
/*-----------------------------------------------------------------------*/

/*
 * XXX (2005-12-07): If no GUI is available (e.g. over a SSH session),
 * you won't be prompted for credentials with which to unlock your
 * keychain.  Apple recognizes lack of TTY prompting as a known
 * problem.
 *
 *
 * XXX (2005-12-07): SecKeychainSetUserInteractionAllowed(FALSE) does
 * not appear to actually prevent all user interaction.  Specifically,
 * if the executable changes (for example, if it is rebuilt), the
 * system prompts the user to okay the use of the new executable.
 *
 * Worse than that, the interactivity setting is global per app (not
 * process/thread), meaning that there is a race condition in the
 * implementation below between calls to
 * SecKeychainSetUserInteractionAllowed() when multiple instances of
 * the same Subversion auth provider-based app run concurrently.
 */

/* Implementation of svn_auth__password_set_t that stores
   the password in the OS X KeyChain. */
static svn_boolean_t
keychain_password_set(apr_hash_t *creds,
                      const char *realmstring,
                      const char *username,
                      const char *password,
                      apr_hash_t *parameters,
                      svn_boolean_t non_interactive,
                      apr_pool_t *pool)
{
  OSStatus status;
  SecKeychainItemRef item;

  if (non_interactive)
    SecKeychainSetUserInteractionAllowed(FALSE);

  status = SecKeychainFindGenericPassword(NULL, strlen(realmstring),
                                          realmstring, username == NULL
                                            ? 0
                                            : strlen(username),
                                          username, 0, NULL, &item);
  if (status)
    {
      if (status == errSecItemNotFound)
        status = SecKeychainAddGenericPassword(NULL, strlen(realmstring),
                                               realmstring, username == NULL
                                                 ? 0
                                                 : strlen(username),
                                               username, strlen(password),
                                               password, NULL);
    }
  else
    {
      status = SecKeychainItemModifyAttributesAndData(item, NULL,
                                                      strlen(password),
                                                      password);
      CFRelease(item);
    }

  if (non_interactive)
    SecKeychainSetUserInteractionAllowed(TRUE);

  return status == 0;
}

/* Implementation of svn_auth__password_get_t that retrieves
   the password from the OS X KeyChain. */
static svn_boolean_t
keychain_password_get(const char **password,
                      apr_hash_t *creds,
                      const char *realmstring,
                      const char *username,
                      apr_hash_t *parameters,
                      svn_boolean_t non_interactive,
                      apr_pool_t *pool)
{
  OSStatus status;
  UInt32 length;
  void *data;

  if (non_interactive)
    SecKeychainSetUserInteractionAllowed(FALSE);

  status = SecKeychainFindGenericPassword(NULL, strlen(realmstring),
                                          realmstring, username == NULL
                                            ? 0
                                            : strlen(username),
                                          username, &length, &data, NULL);

  if (non_interactive)
    SecKeychainSetUserInteractionAllowed(TRUE);

  if (status != 0)
    return FALSE;

  *password = apr_pstrmemdup(pool, data, length);
  SecKeychainItemFreeContent(NULL, data);
  return TRUE;
}

/* Get cached encrypted credentials from the simple provider's cache. */
static svn_error_t *
keychain_simple_first_creds(void **credentials,
                            void **iter_baton,
                            void *provider_baton,
                            apr_hash_t *parameters,
                            const char *realmstring,
                            apr_pool_t *pool)
{
  return svn_auth__simple_first_creds_helper(credentials,
                                             iter_baton,
                                             provider_baton,
                                             parameters,
                                             realmstring,
                                             keychain_password_get,
                                             SVN_AUTH__KEYCHAIN_PASSWORD_TYPE,
                                             pool);
}

/* Save encrypted credentials to the simple provider's cache. */
static svn_error_t *
keychain_simple_save_creds(svn_boolean_t *saved,
                           void *credentials,
                           void *provider_baton,
                           apr_hash_t *parameters,
                           const char *realmstring,
                           apr_pool_t *pool)
{
  return svn_auth__simple_save_creds_helper(saved, credentials,
                                            provider_baton,
                                            parameters,
                                            realmstring,
                                            keychain_password_set,
                                            SVN_AUTH__KEYCHAIN_PASSWORD_TYPE,
                                            pool);
}

static const svn_auth_provider_t keychain_simple_provider = {
  SVN_AUTH_CRED_SIMPLE,
  keychain_simple_first_creds,
  NULL,
  keychain_simple_save_creds
};

/* Get cached encrypted credentials from the ssl client cert password
   provider's cache. */
static svn_error_t *
keychain_ssl_client_cert_pw_first_creds(void **credentials,
                                        void **iter_baton,
                                        void *provider_baton,
                                        apr_hash_t *parameters,
                                        const char *realmstring,
                                        apr_pool_t *pool)
{
  return svn_auth__ssl_client_cert_pw_file_first_creds_helper
           (credentials,
            iter_baton, provider_baton,
            parameters, realmstring,
            keychain_password_get,
            SVN_AUTH__KEYCHAIN_PASSWORD_TYPE,
            pool);
}

/* Save encrypted credentials to the ssl client cert password provider's
   cache. */
static svn_error_t *
keychain_ssl_client_cert_pw_save_creds(svn_boolean_t *saved,
                                       void *credentials,
                                       void *provider_baton,
                                       apr_hash_t *parameters,
                                       const char *realmstring,
                                       apr_pool_t *pool)
{
  return svn_auth__ssl_client_cert_pw_file_save_creds_helper
           (saved, credentials,
            provider_baton, parameters,
            realmstring,
            keychain_password_set,
            SVN_AUTH__KEYCHAIN_PASSWORD_TYPE,
            pool);
}

static const svn_auth_provider_t keychain_ssl_client_cert_pw_provider = {
  SVN_AUTH_CRED_SSL_CLIENT_CERT_PW,
  keychain_ssl_client_cert_pw_first_creds,
  NULL,
  keychain_ssl_client_cert_pw_save_creds
};


/* Public API */
void
svn_auth_get_keychain_simple_provider(svn_auth_provider_object_t **provider,
                                      apr_pool_t *pool)
{
  svn_auth_provider_object_t *po = apr_pcalloc(pool, sizeof(*po));

  po->vtable = &keychain_simple_provider;
  *provider = po;
}

void
svn_auth_get_keychain_ssl_client_cert_pw_provider
  (svn_auth_provider_object_t **provider,
   apr_pool_t *pool)
{
  svn_auth_provider_object_t *po = apr_pcalloc(pool, sizeof(*po));

  po->vtable = &keychain_ssl_client_cert_pw_provider;
  *provider = po;
}
#endif /* SVN_HAVE_KEYCHAIN_SERVICES */
