/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEYRING_H
#define _KEYRING_H


#include <Key.h>
#include <Message.h>


class Keyring {
public:
									Keyring(const char* name,
										const BMessage* keyMessage = NULL);
									~Keyring();

		const char*					Name() const { return fName; }
		status_t					ReadFromMessage(const BMessage& message);
		status_t					WriteToMessage(BMessage& message);

		status_t					Access(const BMessage& keyMessage);
		void						RevokeAccess();
		bool						IsAccessible() const;
		const BMessage&				KeyMessage() const;

		status_t					GetNextApplication(uint32& cookie,
										BString& signature, BString& path);
		status_t					FindApplication(const char* signature,
										const char* path, BMessage& appMessage);
		status_t					AddApplication(const char* signature,
										const BMessage& appMessage);
		status_t					RemoveApplication(const char* signature,
										const char* path);

		status_t					FindKey(const BString& identifier,
										const BString& secondaryIdentifier,
										bool secondaryIdentifierOptional,
										BMessage* _foundKeyMessage) const;
		status_t					FindKey(BKeyType type, BKeyPurpose purpose,
										uint32 index,
										BMessage& _foundKeyMessage) const;

		status_t					AddKey(const BString& identifier,
										const BString& secondaryIdentifier,
										const BMessage& keyMessage);
		status_t					RemoveKey(const BString& identifier,
										const BMessage& keyMessage);

static	int							Compare(const Keyring* one,
										const Keyring* two);
static	int							Compare(const BString* name,
										const Keyring* keyring);

private:
		status_t					_EncryptToFlatBuffer();
		status_t					_DecryptFromFlatBuffer();

		BString						fName;
		BMallocIO					fFlatBuffer;
		BMessage					fData;
		BMessage					fApplications;
		BMessage					fKeyMessage;
		bool						fAccessible;
		bool						fModified;
};


#endif // _KEYRING_H
