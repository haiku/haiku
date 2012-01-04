/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "KeyStoreServer.h"
#include "Keyring.h"

#include <KeyStoreDefs.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include <new>

#include <stdio.h>


using namespace BPrivate;


KeyStoreServer::KeyStoreServer()
	:
	BApplication(kKeyStoreServerSignature),
	fDefaultKeyring(NULL),
	fKeyrings(20, true)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	BDirectory settingsDir(path.Path());
	path.Append("system");
	if (!settingsDir.Contains(path.Path()))
		settingsDir.CreateDirectory(path.Path(), NULL);

	settingsDir.SetTo(path.Path());
	path.Append("keystore");
	if (!settingsDir.Contains(path.Path()))
		settingsDir.CreateDirectory(path.Path(), NULL);

	settingsDir.SetTo(path.Path());
	path.Append("keystore_database");

	fKeyStoreFile.SetTo(path.Path(), B_READ_WRITE
		| (settingsDir.Contains(path.Path()) ? 0 : B_CREATE_FILE));

	_ReadKeyStoreDatabase();

	if (fDefaultKeyring == NULL)
		fDefaultKeyring = new(std::nothrow) Keyring("", BMessage());
}


KeyStoreServer::~KeyStoreServer()
{
}


void
KeyStoreServer::MessageReceived(BMessage* message)
{
	BMessage reply;
	status_t result = B_UNSUPPORTED;

	// Resolve the keyring for the relevant messages.
	Keyring* keyring = NULL;
	switch (message->what) {
		case KEY_STORE_GET_KEY:
		case KEY_STORE_GET_NEXT_KEY:
		case KEY_STORE_ADD_KEY:
		case KEY_STORE_REMOVE_KEY:
		case KEY_STORE_IS_KEYRING_ACCESSIBLE:
		case KEY_STORE_REVOKE_ACCESS:
		{
			BString keyringName;
			if (message->FindString("keyring", &keyringName) != B_OK)
				keyringName = "";

			keyring = _FindKeyring(keyringName);
			if (keyring == NULL) {
				result = B_BAD_VALUE;
				message->what = 0;
					// So that we don't do anything in the second switch.
				break;
			}

			break;
		}
	}

	switch (message->what) {
		case KEY_STORE_GET_KEY:
		{
			BString identifier;
			if (message->FindString("identifier", &identifier) != B_OK) {
				result = B_BAD_VALUE;
				break;
			}

			bool secondaryIdentifierOptional;
			if (message->FindBool("secondaryIdentifierOptional",
					&secondaryIdentifierOptional) != B_OK) {
				secondaryIdentifierOptional = false;
			}

			BString secondaryIdentifier;
			if (message->FindString("secondaryIdentifier",
					&secondaryIdentifier) != B_OK) {
				secondaryIdentifier = "";
				secondaryIdentifierOptional = true;
			}

			BMessage keyMessage;
			result = keyring->FindKey(identifier, secondaryIdentifier,
				secondaryIdentifierOptional, &keyMessage);
			if (result == B_OK)
				reply.AddMessage("key", &keyMessage);
			break;
		}

		case KEY_STORE_GET_NEXT_KEY:
		{
			BKeyType type;
			BKeyPurpose purpose;
			uint32 cookie;
			if (message->FindUInt32("type", (uint32*)&type) != B_OK
				|| message->FindUInt32("purpose", (uint32*)&purpose) != B_OK
				|| message->FindUInt32("cookie", &cookie) != B_OK) {
				result = B_BAD_VALUE;
				break;
			}

			BMessage keyMessage;
			result = keyring->FindKey(type, purpose, cookie, keyMessage);
			if (result == B_OK) {
				cookie++;
				reply.AddUInt32("cookie", cookie);
				reply.AddMessage("key", &keyMessage);
			}

			break;
		}

		case KEY_STORE_ADD_KEY:
		{
			BMessage keyMessage;
			BString identifier;
			if (message->FindMessage("key", &keyMessage) != B_OK
				|| keyMessage.FindString("identifier", &identifier) != B_OK) {
				result = B_BAD_VALUE;
				break;
			}

			BString secondaryIdentifier;
			if (keyMessage.FindString("secondaryIdentifier",
					&secondaryIdentifier) != B_OK) {
				secondaryIdentifier = "";
			}

			result = keyring->AddKey(identifier, secondaryIdentifier, keyMessage);
			if (result == B_OK)
				_WriteKeyStoreDatabase();

			break;
		}

		case KEY_STORE_REMOVE_KEY:
		{
			BMessage keyMessage;
			BString identifier;
			if (message->FindMessage("key", &keyMessage) != B_OK
				|| keyMessage.FindString("identifier", &identifier) != B_OK) {
				result = B_BAD_VALUE;
				break;
			}

			result = keyring->RemoveKey(identifier, keyMessage);
			if (result == B_OK)
				_WriteKeyStoreDatabase();

			break;
		}

		case KEY_STORE_ADD_KEYRING:
		{
			BMessage keyMessage;
			BString keyring;
			if (message->FindString("keyring", &keyring) != B_OK
				|| message->FindMessage("key", &keyMessage) != B_OK) {
				result = B_BAD_VALUE;
				break;
			}

			result = _AddKeyring(keyring, keyMessage);
			if (result == B_OK)
				_WriteKeyStoreDatabase();

			break;
		}

		case KEY_STORE_REMOVE_KEYRING:
		{
			BString keyringName;
			if (message->FindString("keyring", &keyringName) != B_OK)
				keyringName = "";

			result = _RemoveKeyring(keyringName);
			if (result == B_OK)
				_WriteKeyStoreDatabase();

			break;
		}

		case KEY_STORE_GET_NEXT_KEYRING:
		{
			uint32 cookie;
			if (message->FindUInt32("cookie", &cookie) != B_OK) {
				result = B_BAD_VALUE;
				break;
			}

			if (cookie == 0)
				keyring = fDefaultKeyring;
			else
				keyring = fKeyrings.ItemAt(cookie - 1);

			if (keyring == NULL) {
				result = B_ENTRY_NOT_FOUND;
				break;
			}

			cookie++;
			reply.AddUInt32("cookie", cookie);
			reply.AddString("keyring", keyring->Name());
			result = B_OK;
			break;
		}

		case KEY_STORE_IS_KEYRING_ACCESSIBLE:
		{
			reply.AddBool("accessible", keyring->IsAccessible());
			result = B_OK;
		}

		case 0:
		{
			// Just the error case from above.
			break;
		}

		default:
		{
			printf("unknown message received: %" B_PRIu32 " \"%.4s\"\n",
				message->what, (const char*)&message->what);
			break;
		}
	}

	if (message->IsSourceWaiting()) {
		if (result == B_OK)
			reply.what = KEY_STORE_SUCCESS;
		else {
			reply.what = KEY_STORE_RESULT;
			reply.AddInt32("result", result);
		}

		message->SendReply(&reply);
	}
}


status_t
KeyStoreServer::_ReadKeyStoreDatabase()
{
	BMessage keyrings;
	status_t result = keyrings.Unflatten(&fKeyStoreFile);
	if (result != B_OK) {
		printf("failed to read keystore database\n");
		_WriteKeyStoreDatabase();
		return result;
	}

	int32 index = 0;
	char* keyringName = NULL;
	while (keyrings.GetInfo(B_MESSAGE_TYPE, index++, &keyringName,
			NULL) == B_OK) {

		BMessage keyringData;
		if (keyrings.FindMessage(keyringName, &keyringData) != B_OK) {
			printf("failed to retrieve keyring data for keyring \"%s\"\n",
				keyringName);
			continue;
		}

		Keyring* keyring = new(std::nothrow) Keyring(keyringName, keyringData);
		if (keyring == NULL) {
			printf("no memory for allocating keyring \"%s\"\n", keyringName);
			continue;
		}

		if (strlen(keyringName) == 0)
			fDefaultKeyring = keyring;
		else
			fKeyrings.BinaryInsert(keyring, &Keyring::Compare);
	}

	return B_OK;
}


status_t
KeyStoreServer::_WriteKeyStoreDatabase()
{
	fKeyStoreFile.SetSize(0);
	fKeyStoreFile.Seek(0, SEEK_SET);

	BMessage keyrings;
	if (fDefaultKeyring != NULL)
		keyrings.AddMessage("", &fDefaultKeyring->Data());

	for (int32 i = 0; i < fKeyrings.CountItems(); i++) {
		Keyring* keyring = fKeyrings.ItemAt(i);
		if (keyring == NULL)
			continue;

		keyrings.AddMessage(keyring->Name(), &keyring->Data());
	}

	return keyrings.Flatten(&fKeyStoreFile);
}


Keyring*
KeyStoreServer::_FindKeyring(const BString& name)
{
	if (name.IsEmpty())
		return fDefaultKeyring;

	return fKeyrings.BinarySearchByKey(name, &Keyring::Compare);
}


status_t
KeyStoreServer::_AddKeyring(const BString& name, const BMessage& keyMessage)
{
	if (_FindKeyring(name) != NULL)
		return B_NAME_IN_USE;

	Keyring* keyring = new(std::nothrow) Keyring(name, BMessage());
	if (keyring == NULL)
		return B_NO_MEMORY;

	if (!fKeyrings.BinaryInsert(keyring, &Keyring::Compare)) {
		delete keyring;
		return B_ERROR;
	}

	return B_OK;
}


status_t
KeyStoreServer::_RemoveKeyring(const BString& name)
{
	Keyring* keyring = _FindKeyring(name);
	if (keyring == NULL)
		return B_ENTRY_NOT_FOUND;

	if (keyring == fDefaultKeyring) {
		// The default keyring can't be removed.
		return B_NOT_ALLOWED;
	}

	return fKeyrings.RemoveItem(keyring) ? B_OK : B_ERROR;
}


int
main(int argc, char* argv[])
{
	KeyStoreServer* app = new(std::nothrow) KeyStoreServer();
	if (app == NULL)
		return 1;

	app->Run();
	delete app;
	return 0;
}
