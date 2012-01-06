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

		Keyring*					_FindKeyring(const BString& name);

		status_t					_AddKeyring(const BString& name,
										const BMessage& keyMessage);
		status_t					_RemoveKeyring(const BString& name);

		status_t					_AccessKeyring(Keyring& keyring);

		status_t					_RequestKey(BMessage& keyMessage);

		Keyring*					fDefaultKeyring;
		KeyringList					fKeyrings;
		BFile						fKeyStoreFile;
};


#endif // _KEY_STORE_SERVER_H
