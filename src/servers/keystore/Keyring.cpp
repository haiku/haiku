/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Keyring.h"


Keyring::Keyring(const char* name, const BMessage& data)
	:
	fName(name),
	fData(data)
{
}


Keyring::~Keyring()
{
}


bool
Keyring::IsAccessible()
{
	return true;
}


status_t
Keyring::FindKey(const BString& identifier, const BString& secondaryIdentifier,
	bool secondaryIdentifierOptional, BMessage* _foundKeyMessage)
{
	int32 count;
	type_code type;
	if (fData.GetInfo(identifier, &type, &count) != B_OK)
		return B_ENTRY_NOT_FOUND;

	// We have a matching primary identifier, need to check for the secondary
	// identifier.
	for (int32 i = 0; i < count; i++) {
		BMessage candidate;
		if (fData.FindMessage(identifier, i, &candidate) != B_OK)
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
		return fData.FindMessage(identifier, 0, _foundKeyMessage);
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
Keyring::FindKey(BKeyType type, BKeyPurpose purpose, uint32 index,
	BMessage& _foundKeyMessage)
{
	for (int32 keyIndex = 0;; keyIndex++) {
		int32 count = 0;
		char* identifier = NULL;
		if (fData.GetInfo(B_MESSAGE_TYPE, keyIndex, &identifier, NULL,
				&count) != B_OK) {
			break;
		}

		if (type == B_KEY_TYPE_ANY && purpose == B_KEY_PURPOSE_ANY) {
			// No need to inspect the actual keys.
			if ((int32)index >= count) {
				index -= count;
				continue;
			}

			return fData.FindMessage(identifier, index, &_foundKeyMessage);
		}

		// Go through the keys to check their type and purpose.
		for (int32 subkeyIndex = 0; subkeyIndex < count; subkeyIndex++) {
			BMessage subkey;
			if (fData.FindMessage(identifier, subkeyIndex, &subkey) != B_OK)
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
Keyring::AddKey(const BString& identifier, const BString& secondaryIdentifier,
	const BMessage& keyMessage)
{
	// Check for collisions.
	if (FindKey(identifier, secondaryIdentifier, false, NULL) == B_OK)
		return B_NAME_IN_USE;

	// We're fine, just add the new key.
	return fData.AddMessage(identifier, &keyMessage);
}


status_t
Keyring::RemoveKey(const BString& identifier,
	const BMessage& keyMessage)
{
	int32 count;
	type_code type;
	if (fData.GetInfo(identifier, &type, &count) != B_OK)
		return B_ENTRY_NOT_FOUND;

	for (int32 i = 0; i < count; i++) {
		BMessage candidate;
		if (fData.FindMessage(identifier, i, &candidate) != B_OK)
			return B_ERROR;

		// We require an exact match.
		if (!candidate.HasSameData(keyMessage))
			continue;

		fData.RemoveData(identifier, i);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


int
Keyring::Compare(const Keyring* one, const Keyring* two)
{
	return strcmp(one->Name(), two->Name());
}


int
Keyring::Compare(const BString* name, const Keyring* keyring)
{
	return strcmp(name->String(), keyring->Name());
}
