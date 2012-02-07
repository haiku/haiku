/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "KeyStoreServer.h"

#include "KeyRequestWindow.h"
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


static const char* kKeyringKeysIdentifier = "Keyrings";

static const uint32 kFlagGetKey						= 0x0001;
static const uint32 kFlagEnumerateKeys				= 0x0002;
static const uint32 kFlagAddKey						= 0x0004;
static const uint32 kFlagRemoveKey					= 0x0008;
static const uint32 kFlagAddKeyring					= 0x0010;
static const uint32 kFlagRemoveKeyring				= 0x0020;
static const uint32 kFlagEnumerateKeyrings			= 0x0040;
static const uint32 kFlagSetMasterKey				= 0x0080;
static const uint32 kFlagRemoveMasterKey			= 0x0100;
static const uint32 kFlagAddKeyringsToMaster		= 0x0200;
static const uint32 kFlagRemoveKeyringsFromMaster	= 0x0400;
static const uint32 kFlagEnumerateMasterKeyrings	= 0x0800;
static const uint32 kFlagQueryAccessibility			= 0x1000;
static const uint32 kFlagRevokeAccess				= 0x2000;
static const uint32 kFlagEnumerateApplications		= 0x4000;
static const uint32 kFlagRemoveApplications			= 0x8000;

static const uint32 kDefaultAppFlags = kFlagGetKey | kFlagEnumerateKeys
	| kFlagAddKey | kFlagRemoveKey | kFlagAddKeyring | kFlagRemoveKeyring
	| kFlagEnumerateKeyrings | kFlagSetMasterKey | kFlagRemoveMasterKey
	| kFlagAddKeyringsToMaster | kFlagRemoveKeyringsFromMaster
	| kFlagEnumerateMasterKeyrings | kFlagQueryAccessibility
	| kFlagQueryAccessibility | kFlagRevokeAccess | kFlagEnumerateApplications
	| kFlagRemoveApplications;


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
		fDefaultKeyring = new(std::nothrow) Keyring("");
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
		case KEY_STORE_ADD_KEYRING_TO_MASTER:
		case KEY_STORE_REMOVE_KEYRING_FROM_MASTER:
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

			switch (message->what) {
				case KEY_STORE_GET_KEY:
				case KEY_STORE_GET_NEXT_KEY:
				case KEY_STORE_ADD_KEY:
				case KEY_STORE_REMOVE_KEY:
				case KEY_STORE_ADD_KEYRING_TO_MASTER:
				{
					// These need keyring access to do anything.
					while (!keyring->IsAccessible()) {
						status_t accessResult = _AccessKeyring(*keyring);
						if (accessResult != B_OK) {
							result = accessResult;
							message->what = 0;
							break;
						}
					}
				}
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
			break;
		}

		case KEY_STORE_REVOKE_ACCESS:
		{
			keyring->RevokeAccess();
			result = B_OK;
			break;
		}

		case KEY_STORE_ADD_KEYRING_TO_MASTER:
		case KEY_STORE_REMOVE_KEYRING_FROM_MASTER:
		{
			// We also need access to the default keyring.
			while (!fDefaultKeyring->IsAccessible()) {
				status_t accessResult = _AccessKeyring(*fDefaultKeyring);
				if (accessResult != B_OK) {
					result = accessResult;
					message->what = 0;
					break;
				}
			}

			if (message->what == 0)
				break;

			BString secondaryIdentifier = keyring->Name();
			BMessage keyMessage = keyring->KeyMessage();
			keyMessage.RemoveName("identifier");
			keyMessage.AddString("identifier", kKeyringKeysIdentifier);
			keyMessage.RemoveName("secondaryIdentifier");
			keyMessage.AddString("secondaryIdentifier", secondaryIdentifier);

			switch (message->what) {
				case KEY_STORE_ADD_KEYRING_TO_MASTER:
					result = fDefaultKeyring->AddKey(kKeyringKeysIdentifier,
						secondaryIdentifier, keyMessage);
					break;

				case KEY_STORE_REMOVE_KEYRING_FROM_MASTER:
					result = fDefaultKeyring->RemoveKey(kKeyringKeysIdentifier,
						keyMessage);
					break;
			}

			if (result == B_OK)
				_WriteKeyStoreDatabase();

			break;
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
	while (keyrings.GetInfo(B_RAW_TYPE, index++, &keyringName, NULL) == B_OK) {
		Keyring* keyring = new(std::nothrow) Keyring(keyringName);
		if (keyring == NULL) {
			printf("no memory for allocating keyring \"%s\"\n", keyringName);
			continue;
		}

		status_t result = keyring->ReadFromMessage(keyrings);
		if (result != B_OK) {
			printf("failed to read keyring \"%s\" from data\n", keyringName);
			delete keyring;
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
		fDefaultKeyring->WriteToMessage(keyrings);

	for (int32 i = 0; i < fKeyrings.CountItems(); i++) {
		Keyring* keyring = fKeyrings.ItemAt(i);
		if (keyring == NULL)
			continue;

		status_t result = keyring->WriteToMessage(keyrings);
		if (result != B_OK)
			return result;
	}

	return keyrings.Flatten(&fKeyStoreFile);
}


uint32
KeyStoreServer::_AccessFlagsFor(uint32 command) const
{
	switch (command) {
		case KEY_STORE_GET_KEY:
			return kFlagGetKey;
		case KEY_STORE_GET_NEXT_KEY:
			return kFlagEnumerateKeys;
		case KEY_STORE_ADD_KEY:
			return kFlagAddKey;
		case KEY_STORE_REMOVE_KEY:
			return kFlagRemoveKey;
		case KEY_STORE_ADD_KEYRING:
			return kFlagAddKeyring;
		case KEY_STORE_REMOVE_KEYRING:
			return kFlagRemoveKeyring;
		case KEY_STORE_GET_NEXT_KEYRING:
			return kFlagEnumerateKeyrings;
		case KEY_STORE_SET_MASTER_KEY:
			return kFlagSetMasterKey;
		case KEY_STORE_REMOVE_MASTER_KEY:
			return kFlagRemoveMasterKey;
		case KEY_STORE_ADD_KEYRING_TO_MASTER:
			return kFlagAddKeyringsToMaster;
		case KEY_STORE_REMOVE_KEYRING_FROM_MASTER:
			return kFlagRemoveKeyringsFromMaster;
		case KEY_STORE_GET_NEXT_MASTER_KEYRING:
			return kFlagEnumerateMasterKeyrings;
		case KEY_STORE_IS_KEYRING_ACCESSIBLE:
			return kFlagQueryAccessibility;
		case KEY_STORE_REVOKE_ACCESS:
			return kFlagRevokeAccess;
		case KEY_STORE_GET_NEXT_APPLICATION:
			return kFlagEnumerateApplications;
		case KEY_STORE_REMOVE_APPLICATION:
			return kFlagRemoveApplications;
	}

	return 0;
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

	Keyring* keyring = new(std::nothrow) Keyring(name, &keyMessage);
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


status_t
KeyStoreServer::_AccessKeyring(Keyring& keyring)
{
	// If we are accessing a keyring that has been added to master access we
	// get the key from the default keyring and unlock with that.
	BMessage keyMessage;
	if (&keyring != fDefaultKeyring && fDefaultKeyring->IsAccessible()) {
		if (fDefaultKeyring->FindKey(kKeyringKeysIdentifier, keyring.Name(),
				false, &keyMessage) == B_OK) {
			// We found a key for this keyring, try to access with it.
			if (keyring.Access(keyMessage) == B_OK)
				return B_OK;
		}
	}

	// No key, we need to request one from the user.
	status_t result = _RequestKey(keyring.Name(), keyMessage);
	if (result != B_OK)
		return result;

	return keyring.Access(keyMessage);
}


status_t
KeyStoreServer::_RequestKey(const BString& keyringName, BMessage& keyMessage)
{
	KeyRequestWindow* requestWindow = new(std::nothrow) KeyRequestWindow();
	if (requestWindow == NULL)
		return B_NO_MEMORY;

	return requestWindow->RequestKey(keyringName, keyMessage);
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
