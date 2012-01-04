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
										const BMessage& data);
									~Keyring();

		const char*					Name() const { return fName; }
		const BMessage&				Data() const { return fData; }

		bool						IsAccessible();

		status_t					FindKey(const BString& identifier,
										const BString& secondaryIdentifier,
										bool secondaryIdentifierOptional,
										BMessage* _foundKeyMessage);
		status_t					FindKey(BKeyType type, BKeyPurpose purpose,
										uint32 index,
										BMessage& _foundKeyMessage);

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
		BString						fName;
		BMessage					fData;
};


#endif // _KEYRING_H
