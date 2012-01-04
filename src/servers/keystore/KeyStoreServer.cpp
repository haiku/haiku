/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "KeyStoreServer.h"

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
	BApplication(kKeyStoreServerSignature)
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
}


KeyStoreServer::~KeyStoreServer()
{
}


void
KeyStoreServer::MessageReceived(BMessage* message)
{
	BMessage reply;
	status_t result = B_MESSAGE_NOT_UNDERSTOOD;

	switch (message->what) {
		case KEY_STORE_GET_KEY:
		{
			result = B_NOT_ALLOWED;
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
			result = _FindKey(type, purpose, cookie, keyMessage);
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

			result = _AddKey(identifier, secondaryIdentifier, keyMessage);
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

#if 0
    // Get thread info that contains our team ID for replying.
	thread_info threadInfo;
	status_t result = get_thread_info(find_thread(NULL), &threadInfo);
	if (result != B_OK)
		return result;

	while (true) {
		KMessage message;
		port_message_info messageInfo;
		status_t error = message.ReceiveFrom(fRequestPort, -1, &messageInfo);
		if (error != B_OK)
			return B_OK;

		bool isRoot = (messageInfo.sender == 0);

		switch (message.What()) {
			case B_REG_GET_PASSWD_DB:
			{
				// lazily build the reply
				try {
					if (fPasswdDBReply->What() == 1) {
						FlatStore store;
						int32 count = fUserDB->WriteFlatPasswdDB(store);
						if (fPasswdDBReply->AddInt32("count", count) != B_OK
							|| fPasswdDBReply->AddData("entries", B_RAW_TYPE,
									store.Buffer(), store.BufferLength(),
									false) != B_OK) {
							error = B_NO_MEMORY;
						}

						fPasswdDBReply->SetWhat(0);
					}
				} catch (...) {
					error = B_NO_MEMORY;
				}

				if (error == B_OK) {
					message.SendReply(fPasswdDBReply, -1, -1, 0, registrarTeam);
				} else {
					fPasswdDBReply->SetTo(1);
					KMessage reply(error);
					message.SendReply(&reply, -1, -1, 0, registrarTeam);
				}

				break;
			}

			case B_REG_GET_GROUP_DB:
			{
				// lazily build the reply
				try {
					if (fGroupDBReply->What() == 1) {
						FlatStore store;
						int32 count = fGroupDB->WriteFlatGroupDB(store);
						if (fGroupDBReply->AddInt32("count", count) != B_OK
							|| fGroupDBReply->AddData("entries", B_RAW_TYPE,
									store.Buffer(), store.BufferLength(),
									false) != B_OK) {
							error = B_NO_MEMORY;
						}

						fGroupDBReply->SetWhat(0);
					}
				} catch (...) {
					error = B_NO_MEMORY;
				}

				if (error == B_OK) {
					message.SendReply(fGroupDBReply, -1, -1, 0, registrarTeam);
				} else {
					fGroupDBReply->SetTo(1);
					KMessage reply(error);
					message.SendReply(&reply, -1, -1, 0, registrarTeam);
				}

				break;
			}


			case B_REG_GET_SHADOW_PASSWD_DB:
			{
				// only root may see the shadow passwd
				if (!isRoot)
					error = EPERM;

				// lazily build the reply
				try {
					if (error == B_OK && fShadowPwdDBReply->What() == 1) {
						FlatStore store;
						int32 count = fUserDB->WriteFlatShadowDB(store);
						if (fShadowPwdDBReply->AddInt32("count", count) != B_OK
							|| fShadowPwdDBReply->AddData("entries", B_RAW_TYPE,
									store.Buffer(), store.BufferLength(),
									false) != B_OK) {
							error = B_NO_MEMORY;
						}

						fShadowPwdDBReply->SetWhat(0);
					}
				} catch (...) {
					error = B_NO_MEMORY;
				}

				if (error == B_OK) {
					message.SendReply(fShadowPwdDBReply, -1, -1, 0,
						registrarTeam);
				} else {
					fShadowPwdDBReply->SetTo(1);
					KMessage reply(error);
					message.SendReply(&reply, -1, -1, 0, registrarTeam);
				}

				break;
			}

			case B_REG_GET_USER:
			{
				User* user = NULL;
				int32 uid;
				const char* name;

				// find user
				if (message.FindInt32("uid", &uid) == B_OK) {
					user = fUserDB->UserByID(uid);
				} else if (message.FindString("name", &name) == B_OK) {
					user = fUserDB->UserByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				if (error == B_OK && user == NULL)
					error = ENOENT;

				bool getShadowPwd = message.GetBool("shadow", false);

				// only root may see the shadow passwd
				if (error == B_OK && getShadowPwd && !isRoot)
					error = EPERM;

				// add user to message
				KMessage reply;
				if (error == B_OK)
					error = user->WriteToMessage(reply, getShadowPwd);

				// send reply
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_GET_GROUP:
			{
				Group* group = NULL;
				int32 gid;
				const char* name;

				// find group
				if (message.FindInt32("gid", &gid) == B_OK) {
					group = fGroupDB->GroupByID(gid);
				} else if (message.FindString("name", &name) == B_OK) {
					group = fGroupDB->GroupByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				if (error == B_OK && group == NULL)
					error = ENOENT;

				// add group to message
				KMessage reply;
				if (error == B_OK)
					error = group->WriteToMessage(reply);

				// send reply
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_GET_USER_GROUPS:
			{
				// get user name
				const char* name;
				int32 maxCount;
				if (message.FindString("name", &name) != B_OK
					|| message.FindInt32("max count", &maxCount) != B_OK
					|| maxCount <= 0) {
					error = B_BAD_VALUE;
				}

				// get groups
				gid_t groups[NGROUPS_MAX + 1];
				int32 count = 0;
				if (error == B_OK) {
					maxCount = min_c(maxCount, NGROUPS_MAX + 1);
					count = fGroupDB->GetUserGroups(name, groups, maxCount);
				}

				// add groups to message
				KMessage reply;
				if (error == B_OK) {
					if (reply.AddInt32("count", count) != B_OK
						|| reply.AddData("groups", B_INT32_TYPE,
								groups, min_c(maxCount, count) * sizeof(gid_t),
								false) != B_OK) {
						error = B_NO_MEMORY;
					}
				}

				// send reply
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}

			case B_REG_UPDATE_USER:
			{
				// find user
				User* user = NULL;
				int32 uid;
				const char* name;

				if (message.FindInt32("uid", &uid) == B_OK) {
					user = fUserDB->UserByID(uid);
				} else if (message.FindString("name", &name) == B_OK) {
					user = fUserDB->UserByName(name);
				} else {
					error = B_BAD_VALUE;
				}

				// only can change anything
				if (error == B_OK && !isRoot)
					error = EPERM;

				// check addUser vs. existing user
				bool addUser = message.GetBool("add user", false);
				if (error == B_OK) {
					if (addUser) {
						if (user != NULL)
							error = EEXIST;
					} else if (user == NULL)
						error = ENOENT;
				}

				// apply all changes
				if (error == B_OK) {
					// clone the user object and update it from the message
					User* oldUser = user;
					user = NULL;
					try {
						user = (oldUser != NULL ? new User(*oldUser)
							: new User);
						user->UpdateFromMessage(message);

						// uid and name should remain the same
						if (oldUser != NULL) {
							if (oldUser->UID() != user->UID()
								|| oldUser->Name() != user->Name()) {
								error = B_BAD_VALUE;
							}
						}

						// replace the old user and write DBs to disk
						if (error == B_OK) {
							fUserDB->AddUser(user);
							fUserDB->WriteToDisk();
							fPasswdDBReply->SetTo(1);
							fShadowPwdDBReply->SetTo(1);
						}
					} catch (...) {
						error = B_NO_MEMORY;
					}

					if (error == B_OK)
						delete oldUser;
					else
						delete user;
				}

				// send reply
				KMessage reply;
				reply.SetWhat(error);
				message.SendReply(&reply, -1, -1, 0, registrarTeam);

				break;
			}
			case B_REG_UPDATE_GROUP:
				debug_printf("B_REG_UPDATE_GROUP done: currently unsupported!\n");
				break;
			default:
				debug_printf("REG: invalid message: %lu\n", message.What());

		}
	}
#endif
}


status_t
KeyStoreServer::_ReadKeyStoreDatabase()
{
	status_t result = fDatabase.Unflatten(&fKeyStoreFile);
	if (result != B_OK) {
		printf("failed to read keystore database\n");
		fDatabase.MakeEmpty();
		_WriteKeyStoreDatabase();
		return result;
	}

	return B_OK;
}


status_t
KeyStoreServer::_WriteKeyStoreDatabase()
{
	fKeyStoreFile.SetSize(0);
	fKeyStoreFile.Seek(0, SEEK_SET);
	return fDatabase.Flatten(&fKeyStoreFile);
}


status_t
KeyStoreServer::_FindKey(const BString& identifier,
	const BString& secondaryIdentifier, bool secondaryIdentifierOptional,
	BMessage* _foundKeyMessage)
{
	int32 count;
	type_code type;
	if (fDatabase.GetInfo(identifier, &type, &count) != B_OK)
		return B_ENTRY_NOT_FOUND;

	// We have a matching primary identifier, need to check for the secondary
	// identifier.
	for (int32 i = 0; i < count; i++) {
		BMessage candidate;
		if (fDatabase.FindMessage(identifier, i, &candidate) != B_OK)
			return B_ERROR;

		BString candidateIdentifier;
		if (candidate.FindString("secondaryIdentifier",
				&candidateIdentifier) != B_OK) {
			candidateIdentifier = "";
		}

		if (candidateIdentifier == secondaryIdentifier) {
			if (_foundKeyMessage != NULL)
				*_foundKeyMessage = candidate;
			return B_OK;
		}
	}

	// We didn't find an exact match.
	if (secondaryIdentifierOptional) {
		if (_foundKeyMessage == NULL)
			return B_OK;

		// The secondary identifier is optional, so we just return the
		// first entry.
		return fDatabase.FindMessage(identifier, 0, _foundKeyMessage);
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
KeyStoreServer::_FindKey(BKeyType type, BKeyPurpose purpose, uint32 index,
	BMessage& _foundKeyMessage)
{
	for (int32 keyIndex = 0;; keyIndex++) {
		int32 count = 0;
		char* identifier = NULL;
		if (fDatabase.GetInfo(B_MESSAGE_TYPE, keyIndex, &identifier, NULL,
				&count) != B_OK) {
			break;
		}

		if (type == B_KEY_TYPE_ANY && purpose == B_KEY_PURPOSE_ANY) {
			// No need to inspect the actual keys.
			if ((int32)index >= count) {
				index -= count;
				continue;
			}

			return fDatabase.FindMessage(identifier, index, &_foundKeyMessage);
		}

		// Go through the keys to check their type and purpose.
		for (int32 subkeyIndex = 0; subkeyIndex < count; subkeyIndex++) {
			BMessage subkey;
			if (fDatabase.FindMessage(identifier, subkeyIndex, &subkey) != B_OK)
				return B_ERROR;

			bool match = true;
			if (type != B_KEY_TYPE_ANY) {
				BKeyType subkeyType;
				if (subkey.FindUInt32("type", (uint32*)&subkeyType) != B_OK)
					return B_ERROR;

				match = subkeyType == type;
			}

			if (match && purpose != B_KEY_PURPOSE_ANY) {
				BKeyPurpose subkeyPurpose;
				if (subkey.FindUInt32("purpose", (uint32*)&subkeyPurpose)
						!= B_OK) {
					return B_ERROR;
				}

				match = subkeyPurpose == purpose;
			}

			if (match) {
				if (index == 0) {
					_foundKeyMessage = subkey;
					return B_OK;
				}

				index--;
			}
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
KeyStoreServer::_AddKey(const BString& identifier,
	const BString& secondaryIdentifier, const BMessage& keyMessage)
{
	// Check for collisions.
	if (_FindKey(identifier, secondaryIdentifier, false, NULL) == B_OK)
		return B_NAME_IN_USE;

	// We're fine, just add the new key.
	fDatabase.AddMessage(identifier, &keyMessage);
	return _WriteKeyStoreDatabase();
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
