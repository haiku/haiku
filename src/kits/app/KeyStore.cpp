/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KeyStore.h>

#include <KeyStoreDefs.h>

#include <Messenger.h>
#include <Roster.h>


using namespace BPrivate;


BKeyStore::BKeyStore()
{
}


BKeyStore::~BKeyStore()
{
}


// #pragma mark - Key handling


status_t
BKeyStore::GetKey(BKeyType type, const char* identifier, BKey& key)
{
	return GetKey(NULL, type, identifier, NULL, true, key);
}


status_t
BKeyStore::GetKey(BKeyType type, const char* identifier,
	const char* secondaryIdentifier, BKey& key)
{
	return GetKey(NULL, type, identifier, secondaryIdentifier, true, key);
}


status_t
BKeyStore::GetKey(BKeyType type, const char* identifier,
	const char* secondaryIdentifier, bool secondaryIdentifierOptional,
	BKey& key)
{
	return GetKey(NULL, type, identifier, secondaryIdentifier,
		secondaryIdentifierOptional, key);
}


status_t
BKeyStore::GetKey(const char* keyring, BKeyType type, const char* identifier,
	BKey& key)
{
	return GetKey(keyring, type, identifier, NULL, true, key);
}


status_t
BKeyStore::GetKey(const char* keyring, BKeyType type, const char* identifier,
	const char* secondaryIdentifier, BKey& key)
{
	return GetKey(keyring, type, identifier, secondaryIdentifier, true, key);
}


status_t
BKeyStore::GetKey(const char* keyring, BKeyType type, const char* identifier,
	const char* secondaryIdentifier, bool secondaryIdentifierOptional,
	BKey& key)
{
	BMessage message(KEY_STORE_GET_KEY);
	message.AddString("keyring", keyring);
	message.AddUInt32("type", type);
	message.AddString("identifier", identifier);
	message.AddString("secondaryIdentifier", secondaryIdentifier);
	message.AddBool("secondaryIdentifierOptional", secondaryIdentifierOptional);

	BMessage reply;
	status_t result = _SendKeyMessage(message, &reply);
	if (result != B_OK)
		return result;

	BMessage keyMessage;
	if (reply.FindMessage("key", &keyMessage) != B_OK)
		return B_ERROR;

	return key.Unflatten(keyMessage);
}


status_t
BKeyStore::AddKey(const BKey& key)
{
	return AddKey(NULL, key);
}


status_t
BKeyStore::AddKey(const char* keyring, const BKey& key)
{
	BMessage keyMessage;
	if (key.Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_ADD_KEY);
	message.AddString("keyring", keyring);
	message.AddMessage("key", &keyMessage);

	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::RemoveKey(const BKey& key)
{
	return RemoveKey(NULL, key);
}


status_t
BKeyStore::RemoveKey(const char* keyring, const BKey& key)
{
	BMessage keyMessage;
	if (key.Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_REMOVE_KEY);
	message.AddString("keyring", keyring);
	message.AddMessage("key", &keyMessage);

	return _SendKeyMessage(message, NULL);
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
	return GetNextKey(keyring, B_KEY_TYPE_ANY, B_KEY_PURPOSE_ANY, cookie, key);
}


status_t
BKeyStore::GetNextKey(const char* keyring, BKeyType type, BKeyPurpose purpose,
	uint32& cookie, BKey& key)
{
	BMessage message(KEY_STORE_GET_NEXT_KEY);
	message.AddString("keyring", keyring);
	message.AddUInt32("type", type);
	message.AddUInt32("purpose", purpose);
	message.AddUInt32("cookie", cookie);

	BMessage reply;
	status_t result = _SendKeyMessage(message, &reply);
	if (result != B_OK)
		return result;

	BMessage keyMessage;
	if (reply.FindMessage("key", &keyMessage) != B_OK)
		return B_ERROR;

	reply.FindUInt32("cookie", &cookie);
	return key.Unflatten(keyMessage);
}


// #pragma mark - Keyrings


status_t
BKeyStore::AddKeyring(const char* keyring)
{
	BMessage message(KEY_STORE_ADD_KEYRING);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::RemoveKeyring(const char* keyring)
{
	BMessage message(KEY_STORE_REMOVE_KEYRING);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::GetNextKeyring(uint32& cookie, BString& keyring)
{
	BMessage message(KEY_STORE_GET_NEXT_KEYRING);
	message.AddUInt32("cookie", cookie);

	BMessage reply;
	status_t result = _SendKeyMessage(message, &reply);
	if (result != B_OK)
		return result;

	if (reply.FindString("keyring", &keyring) != B_OK)
		return B_ERROR;

	reply.FindUInt32("cookie", &cookie);
	return B_OK;
}


status_t
BKeyStore::SetUnlockKey(const char* keyring, const BKey& key)
{
	BMessage keyMessage;
	if (key.Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_SET_UNLOCK_KEY);
	message.AddString("keyring", keyring);
	message.AddMessage("key", &keyMessage);

	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::RemoveUnlockKey(const char* keyring)
{
	BMessage message(KEY_STORE_REMOVE_UNLOCK_KEY);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


// #pragma mark - Master key


status_t
BKeyStore::SetMasterUnlockKey(const BKey& key)
{
	return SetUnlockKey(NULL, key);
}


status_t
BKeyStore::RemoveMasterUnlockKey()
{
	return RemoveUnlockKey(NULL);
}


status_t
BKeyStore::AddKeyringToMaster(const char* keyring)
{
	BMessage message(KEY_STORE_ADD_KEYRING_TO_MASTER);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::RemoveKeyringFromMaster(const char* keyring)
{
	BMessage message(KEY_STORE_REMOVE_KEYRING_FROM_MASTER);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::GetNextMasterKeyring(uint32& cookie, BString& keyring)
{
	BMessage message(KEY_STORE_GET_NEXT_MASTER_KEYRING);
	message.AddUInt32("cookie", cookie);

	BMessage reply;
	status_t result = _SendKeyMessage(message, &reply);
	if (result != B_OK)
		return result;

	if (reply.FindString("keyring", &keyring) != B_OK)
		return B_ERROR;

	reply.FindUInt32("cookie", &cookie);
	return B_OK;
}


// #pragma mark - Locking


bool
BKeyStore::IsKeyringUnlocked(const char* keyring)
{
	BMessage message(KEY_STORE_IS_KEYRING_UNLOCKED);
	message.AddString("keyring", keyring);

	BMessage reply;
	if (_SendKeyMessage(message, &reply) != B_OK)
		return false;

	bool unlocked;
	if (reply.FindBool("unlocked", &unlocked) != B_OK)
		return false;

	return unlocked;
}


status_t
BKeyStore::LockKeyring(const char* keyring)
{
	BMessage message(KEY_STORE_LOCK_KEYRING);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::LockMasterKeyring()
{
	return LockKeyring(NULL);
}



// #pragma mark - Applications


status_t
BKeyStore::GetNextApplication(uint32& cookie, BString& signature) const
{
	return GetNextApplication(NULL, cookie, signature);
}


status_t
BKeyStore::GetNextApplication(const char* keyring, uint32& cookie,
	BString& signature) const
{
	BMessage message(KEY_STORE_GET_NEXT_APPLICATION);
	message.AddString("keyring", keyring);
	message.AddUInt32("cookie", cookie);

	BMessage reply;
	status_t result = _SendKeyMessage(message, &reply);
	if (result != B_OK)
		return result;

	if (reply.FindString("signature", &signature) != B_OK)
		return B_ERROR;

	reply.FindUInt32("cookie", &cookie);
	return B_OK;
}


status_t
BKeyStore::RemoveApplication(const char* signature)
{
	return RemoveApplication(NULL, signature);
}


status_t
BKeyStore::RemoveApplication(const char* keyring, const char* signature)
{
	BMessage message(KEY_STORE_REMOVE_APPLICATION);
	message.AddString("keyring", keyring);
	message.AddString("signature", signature);

	return _SendKeyMessage(message, NULL);
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


// #pragma mark - Private functions


status_t
BKeyStore::_SendKeyMessage(BMessage& message, BMessage* reply) const
{
	BMessage localReply;
	if (reply == NULL)
		reply = &localReply;

	BMessenger messenger(kKeyStoreServerSignature);
	if (!messenger.IsValid()) {
		// Try to start the keystore server.
		status_t result = be_roster->Launch(kKeyStoreServerSignature);
		if (result != B_OK && result != B_ALREADY_RUNNING)
			return B_ERROR;

		// Then re-target the messenger and check again.
		messenger.SetTo(kKeyStoreServerSignature);
		if (!messenger.IsValid())
			return B_ERROR;
	}

	if (messenger.SendMessage(&message, reply) != B_OK)
		return B_ERROR;

	if (reply->what != KEY_STORE_SUCCESS) {
		status_t result = B_ERROR;
		if (reply->FindInt32("result", &result) != B_OK)
			return B_ERROR;

		return result;
	}

	return B_OK;
}
