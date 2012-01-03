/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KeyStore.h>


BKeyStore::BKeyStore()
{
}


BKeyStore::~BKeyStore()
{
}


// #pragma mark - Key handling


status_t
BKeyStore::GetKey(BKeyType type, BKeyPurpose purpose, const char* identifier,
	BKey& key)
{
	return GetKey(NULL, type, purpose, identifier, NULL, true, key);
}


status_t
BKeyStore::GetKey(BKeyType type, BKeyPurpose purpose, const char* identifier,
	const char* secondaryIdentifier, BKey& key)
{
	return GetKey(NULL, type, purpose, identifier, secondaryIdentifier, true,
		key);
}


status_t
BKeyStore::GetKey(BKeyType type, BKeyPurpose purpose, const char* identifier,
	const char* secondaryIdentifier, bool secondaryIdentifierOptional,
	BKey& key)
{
	return GetKey(NULL, type, purpose, identifier, secondaryIdentifier,
		secondaryIdentifierOptional, key);
}


status_t
BKeyStore::GetKey(const char* keyring, BKeyType type, BKeyPurpose purpose,
	const char* identifier, BKey& key)
{
	return GetKey(keyring, type, purpose, identifier, NULL, true, key);
}


status_t
BKeyStore::GetKey(const char* keyring, BKeyType type, BKeyPurpose purpose,
	const char* identifier, const char* secondaryIdentifier, BKey& key)
{
	return GetKey(keyring, type, purpose, identifier, secondaryIdentifier, true,
		key);
}


status_t
BKeyStore::GetKey(const char* keyring, BKeyType type, BKeyPurpose purpose,
	const char* identifier, const char* secondaryIdentifier,
	bool secondaryIdentifierOptional, BKey& key)
{
	return B_ERROR;
}


status_t
BKeyStore::AddKey(const BKey& key)
{
	return AddKey(NULL, key);
}


status_t
BKeyStore::AddKey(const char* keyring, const BKey& key)
{
	return B_ERROR;
}


status_t
BKeyStore::RemoveKey(const BKey& key)
{
	return RemoveKey(NULL, key);
}


status_t
BKeyStore::RemoveKey(const char* keyring, const BKey& key)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextKey(uint32& cookie, BKey& key)
{
	return GetNextKey(NULL, cookie, key);
}


status_t
BKeyStore::GetNextKey(BKeyType type, BKeyPurpose purpose, uint32& cookie,
	BKey& key)
{
	return GetNextKey(NULL, type, purpose, cookie, key);
}


status_t
BKeyStore::GetNextKey(const char* keyring, uint32& cookie, BKey& key)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextKey(const char* keyring, BKeyType type, BKeyPurpose purpose,
	uint32& cookie, BKey& key)
{
	return B_ERROR;
}


// #pragma mark - Keyrings


status_t
BKeyStore::AddKeyring(const char* keyring, const BKey& key)
{
	return B_ERROR;
}


status_t
BKeyStore::RemoveKeyring(const char* keyring)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextKeyring(uint32& cookie, BString& keyring)
{
	return B_ERROR;
}


// #pragma mark - Master key


status_t
BKeyStore::SetMasterKey(const BKey& key)
{
	return B_ERROR;
}


status_t
BKeyStore::RemoveMasterKey()
{
	return B_ERROR;
}


status_t
BKeyStore::AddKeyringToMaster(const char* keyring)
{
	return B_ERROR;
}


status_t
BKeyStore::RemoveKeyringFromMaster(const char* keyring)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextMasterKeyring(uint32& cookie, BString& keyring)
{
	return B_ERROR;
}


// #pragma mark - Access


bool
BKeyStore::IsKeyringAccessible(const char* keyring)
{
	return false;
}


status_t
BKeyStore::RevokeAccess(const char* keyring)
{
	return B_ERROR;
}


status_t
BKeyStore::RevokeMasterAccess()
{
	return B_ERROR;
}



// #pragma mark - Applications


status_t
BKeyStore::GetNextApplication(const BKey& key, uint32& cookie,
	BString& signature) const
{
	return B_ERROR;
}


status_t
BKeyStore::RemoveApplication(const BKey& key, const char* signature)
{
	return B_ERROR;
}


// #pragma mark - Service functions


status_t
BKeyStore::GeneratePassword(BPasswordKey& password, size_t length, uint32 flags)
{
	return B_ERROR;
}


float
BKeyStore::PasswordStrength(const char* password)
{
	return 0;
}
