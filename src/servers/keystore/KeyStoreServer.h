/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEY_STORE_SERVER_H
#define _KEY_STORE_SERVER_H


#include <Application.h>
#include <File.h>
#include <Key.h>


class KeyStoreServer : public BApplication {
public:
									KeyStoreServer();
virtual								~KeyStoreServer();

virtual	void						MessageReceived(BMessage* message);

private:
		status_t					_ReadKeyStoreDatabase();
		status_t					_WriteKeyStoreDatabase();

		status_t					_FindKey(const BString& identifier,
										const BString& secondaryIdentifier,
										bool secondaryIdentifierOptional,
										BMessage* _foundKeyMessage);
		status_t					_FindKey(BKeyType type, BKeyPurpose purpose,
										uint32 index,
										BMessage& _foundKeyMessage);

		status_t					_AddKey(const BString& identifier,
										const BString& secondaryIdentifier,
										const BMessage& keyMessage);

		BMessage					fDatabase;
		BFile						fKeyStoreFile;
};


#endif // _KEY_STORE_SERVER_H
