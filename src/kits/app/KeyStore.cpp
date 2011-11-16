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


// #pragma mark - Passwords


status_t
BKeyStore::GetPassword(BPasswordType type, const char* identifier,
	BKey& password)
{
	return GetPassword(NULL, type, identifier, NULL, true, password);
}


status_t
BKeyStore::GetPassword(BPasswordType type, const char* identifier,
	const char* secondaryIdentifier, BKey& password)
{
	return GetPassword(NULL, type, identifier, secondaryIdentifier, true,
		password);
}


status_t
BKeyStore::GetPassword(BPasswordType type, const char* identifier,
	const char* secondaryIdentifier, bool secondaryIdentifierOptional,
	BKey& password)
{
	return GetPassword(NULL, type, identifier, secondaryIdentifier,
		secondaryIdentifierOptional, password);
}


status_t
BKeyStore::GetPassword(const char* keyring, BPasswordType type,
	const char* identifier, BKey& password)
{
	return GetPassword(keyring, type, identifier, NULL, true, password);
}


status_t
BKeyStore::GetPassword(const char* keyring, BPasswordType type,
	const char* identifier, const char* secondaryIdentifier,
	BKey& password)
{
	return GetPassword(keyring, type, identifier, secondaryIdentifier, true,
		password);
}


status_t
BKeyStore::GetPassword(const char* keyring, BPasswordType type,
	const char* identifier, const char* secondaryIdentifier,
	bool secondaryIdentifierOptional, BKey& password)
{
	return B_ERROR;
}


status_t
BKeyStore::RegisterPassword(const BKey& password)
{
	return RegisterPassword(NULL, password);
}


status_t
BKeyStore::RegisterPassword(const char* keyring, const BKey& password)
{
	return B_ERROR;
}


status_t
BKeyStore::UnregisterPassword(const BKey& password)
{
	return UnregisterPassword(NULL, password);
}


status_t
BKeyStore::UnregisterPassword(const char* keyring, const BKey& password)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextPassword(uint32& cookie, BKey& password)
{
	return GetNextPassword(NULL, cookie, password);
}


status_t
BKeyStore::GetNextPassword(BPasswordType type, uint32& cookie,
	BKey& password)
{
	return GetNextPassword(NULL, type, cookie, password);
}


status_t
BKeyStore::GetNextPassword(const char* keyring, uint32& cookie,
	BKey& password)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextPassword(const char* keyring,
	BPasswordType type, uint32& cookie, BKey& password)
{
	return B_ERROR;
}


// #pragma mark - Keyrings


status_t
BKeyStore::RegisterKeyring(const char* keyring, const BKey& password)
{
	return B_ERROR;
}


status_t
BKeyStore::UnregisterKeyring(const char* keyring)
{
	return B_ERROR;
}


status_t
BKeyStore::GetNextKeyring(uint32& cookie, BString& keyring)
{
	return B_ERROR;
}


// #pragma mark - Master password


status_t
BKeyStore::SetMasterPassword(const BKey& password)
{
	return B_ERROR;
}


status_t
BKeyStore::RemoveMasterPassword()
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


// #pragma mark - Service functions


status_t
BKeyStore::GeneratePassword(BKey& password, size_t length, uint32 flags)
{
	return B_ERROR;
}


float
BKeyStore::PasswordStrength(const char* password)
{
	return 0;
}
