/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KeyStore.h>

#include <KeyStoreDefs.h>

#include <Messenger.h>


using namespace BPrivate;


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
	BMessage message(KEY_STORE_GET_KEY);
	message.AddString("keyring", keyring);
	message.AddUInt32("type", type);
	message.AddUInt32("purpose", purpose);
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

	return key._Unflatten(keyMessage);
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
	if (key._Flatten(keyMessage) != B_OK)
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
	if (key._Flatten(keyMessage) != B_OK)
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
	return key._Unflatten(keyMessage);
}


// #pragma mark - Keyrings


status_t
BKeyStore::AddKeyring(const char* keyring, const BKey& key)
{
	BMessage keyMessage;
	if (key._Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_ADD_KEYRING);
	message.AddString("keyring", keyring);
	message.AddMessage("key", &keyMessage);

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


// #pragma mark - Master key


status_t
BKeyStore::SetMasterKey(const BKey& key)
{
	BMessage keyMessage;
	if (key._Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_SET_MASTER_KEY);
	message.AddMessage("key", &keyMessage);

	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::RemoveMasterKey()
{
	BMessage message(KEY_STORE_REMOVE_MASTER_KEY);
	return _SendKeyMessage(message, NULL);
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


// #pragma mark - Access


bool
BKeyStore::IsKeyringAccessible(const char* keyring)
{
	BMessage message(KEY_STORE_IS_KEYRING_ACCESSIBLE);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL) == B_OK;
}


status_t
BKeyStore::RevokeAccess(const char* keyring)
{
	BMessage message(KEY_STORE_REVOKE_ACCESS);
	message.AddString("keyring", keyring);
	return _SendKeyMessage(message, NULL);
}


status_t
BKeyStore::RevokeMasterAccess()
{
	BMessage message(KEY_STORE_REVOKE_MASTER_ACCESS);
	return _SendKeyMessage(message, NULL);
}



// #pragma mark - Applications


status_t
BKeyStore::GetNextApplication(const BKey& key, uint32& cookie,
	BString& signature) const
{
	BMessage keyMessage;
	if (key._Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_GET_NEXT_APPLICATION);
	message.AddMessage("key", &keyMessage);
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
BKeyStore::RemoveApplication(const BKey& key, const char* signature)
{
	BMessage keyMessage;
	if (key._Flatten(keyMessage) != B_OK)
		return B_BAD_VALUE;

	BMessage message(KEY_STORE_REMOVE_APPLICATION);
	message.AddMessage("key", &keyMessage);
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
	if (!messenger.IsValid())
		return B_ERROR;

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
