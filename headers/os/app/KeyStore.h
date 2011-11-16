/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEY_STORE_H
#define _KEY_STORE_H


#include <Key.h>


class BKeyStore {
public:
								BKeyStore();
	virtual						~BKeyStore();

// TODO: -> GetNextPassword() - there can always be more than one key
// with the same identifier/secondaryIdentifier (ie. different username)
			status_t			GetPassword(BPasswordType type,
									const char* identifier, BKey& key);
			status_t			GetPassword(BPasswordType type,
									const char* identifier,
									const char* secondaryIdentifier, BKey& key);
			status_t			GetPassword(BPasswordType type,
									const char* identifier,
									const char* secondaryIdentifier,
									bool secondaryIdentifierOptional,
									BKey& key);

			status_t			GetPassword(const char* keyring,
									BPasswordType type,
									const char* identifier, BKey& key);
			status_t			GetPassword(const char* keyring,
									BPasswordType type,
									const char* identifier,
									const char* secondaryIdentifier, BKey& key);
			status_t			GetPassword(const char* keyring,
									BPasswordType type,
									const char* identifier,
									const char* secondaryIdentifier,
									bool secondaryIdentifierOptional,
									BKey& key);

			status_t			RegisterPassword(const BKey& key);
			status_t			RegisterPassword(const char* keyring,
									const BKey& key);
			status_t			UnregisterPassword(const BKey& key);
			status_t			UnregisterPassword(const char* keyring,
									const BKey& key);

			status_t			GetNextPassword(uint32& cookie, BKey& key);
			status_t			GetNextPassword(BPasswordType type,
									uint32& cookie, BKey& key);
			status_t			GetNextPassword(const char* keyring,
									uint32& cookie, BKey& key);
			status_t			GetNextPassword(const char* keyring,
									BPasswordType type, uint32& cookie,
									BKey& key);

			// Keyrings

			status_t			RegisterKeyring(const char* keyring,
									const BKey& key);
			status_t			UnregisterKeyring(const char* keyring);

			status_t			GetNextKeyring(uint32& cookie,
									BString& keyring);

			// Master key

			status_t			SetMasterPassword(const BKey& key);
			status_t			RemoveMasterPassword();

			status_t			AddKeyringToMaster(const char* keyring);
			status_t			RemoveKeyringFromMaster(const char* keyring);

			status_t			GetNextMasterKeyring(uint32& cookie,
									BString& keyring);

			// Access

			bool				IsKeyringAccessible(const char* keyring);
			status_t			RevokeAccess(const char* keyring);
			status_t			RevokeMasterAccess();

			// Service functions

			status_t			GeneratePassword(BKey& key, size_t length,
									uint32 flags);
			float				PasswordStrength(const char* key);
};


#endif	// _KEY_STORE_H
