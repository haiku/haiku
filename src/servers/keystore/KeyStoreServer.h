/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEY_STORE_SERVER_H
#define _KEY_STORE_SERVER_H


#include <Application.h>
#include <File.h>
#include <Key.h>
#include <ObjectList.h>


struct app_info;
class Keyring;

typedef BObjectList<Keyring> KeyringList;


class KeyStoreServer : public BApplication {
public:
									KeyStoreServer();
virtual								~KeyStoreServer();

virtual	void						MessageReceived(BMessage* message);

private:
		status_t					_ReadKeyStoreDatabase();
		status_t					_WriteKeyStoreDatabase();

		uint32						_AccessFlagsFor(uint32 command) const;
		const char*					_AccessStringFor(uint32 accessFlag) const;
		status_t					_ResolveCallingApp(const BMessage& message,
										app_info& callingAppInfo) const;

		status_t					_ValidateAppAccess(Keyring& keyring,
										const app_info& appInfo,
										uint32 accessFlags);
		status_t					_RequestAppAccess(
										const BString& keyringName,
										const char* signature,
										const char* path,
										const char* accessString, bool appIsNew,
										bool appWasUpdated, uint32 accessFlags,
										bool& allowAlways);

		Keyring*					_FindKeyring(const BString& name);

		status_t					_AddKeyring(const BString& name);
		status_t					_RemoveKeyring(const BString& name);

		status_t					_UnlockKeyring(Keyring& keyring);

		status_t					_RequestKey(const BString& keyringName,
										BMessage& keyMessage);

		Keyring*					fMasterKeyring;
		KeyringList					fKeyrings;
		BFile						fKeyStoreFile;
};


#endif // _KEY_STORE_SERVER_H
