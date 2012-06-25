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

			status_t			GetKey(BKeyType type, const char* identifier,
									BKey& key);
			status_t			GetKey(BKeyType type, const char* identifier,
									const char* secondaryIdentifier, BKey& key);
			status_t			GetKey(BKeyType type, const char* identifier,
									const char* secondaryIdentifier,
									bool secondaryIdentifierOptional,
									BKey& key);

			status_t			GetKey(const char* keyring,
									BKeyType type, const char* identifier,
									BKey& key);
			status_t			GetKey(const char* keyring,
									BKeyType type, const char* identifier,
									const char* secondaryIdentifier, BKey& key);
			status_t			GetKey(const char* keyring,
									BKeyType type, const char* identifier,
									const char* secondaryIdentifier,
									bool secondaryIdentifierOptional,
									BKey& key);

			status_t			AddKey(const BKey& key);
			status_t			AddKey(const char* keyring, const BKey& key);
			status_t			RemoveKey(const BKey& key);
			status_t			RemoveKey(const char* keyring, const BKey& key);

			status_t			GetNextKey(uint32& cookie, BKey& key);
			status_t			GetNextKey(BKeyType type, BKeyPurpose purpose,
									uint32& cookie, BKey& key);
			status_t			GetNextKey(const char* keyring,
									uint32& cookie, BKey& key);
			status_t			GetNextKey(const char* keyring,
									BKeyType type, BKeyPurpose purpose,
									uint32& cookie, BKey& key);

			// Keyrings

			status_t			AddKeyring(const char* keyring);
			status_t			RemoveKeyring(const char* keyring);

			status_t			GetNextKeyring(uint32& cookie,
									BString& keyring);

			status_t			SetUnlockKey(const char* keyring,
									const BKey& key);
			status_t			RemoveUnlockKey(const char* keyring);

			// Master keyring

			status_t			SetMasterUnlockKey(const BKey& key);
			status_t			RemoveMasterUnlockKey();

			status_t			AddKeyringToMaster(const char* keyring);
			status_t			RemoveKeyringFromMaster(const char* keyring);

			status_t			GetNextMasterKeyring(uint32& cookie,
									BString& keyring);

			// Locking

			bool				IsKeyringUnlocked(const char* keyring);
			status_t			LockKeyring(const char* keyring);
			status_t			LockMasterKeyring();

			// Applications

			status_t			GetNextApplication(uint32& cookie,
									BString& signature) const;
			status_t			GetNextApplication(const char* keyring,
									uint32& cookie, BString& signature) const;
			status_t			RemoveApplication(const char* signature);
			status_t			RemoveApplication(const char* keyring,
									const char* signature);

			// Service functions

			status_t			GeneratePassword(BPasswordKey& password,
									size_t length, uint32 flags);
			float				PasswordStrength(const char* password);

private:
			status_t			_SendKeyMessage(BMessage& message,
									BMessage* reply) const;
};


#endif	// _KEY_STORE_H
