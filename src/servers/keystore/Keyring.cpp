/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Keyring.h"


Keyring::Keyring(const char* name, const BMessage* keyMessage)
	:
	fName(name),
	fAccessible(false),
	fModified(false)
{
	if (keyMessage != NULL)
		Access(*keyMessage);
}


Keyring::~Keyring()
{
}


status_t
Keyring::ReadFromMessage(const BMessage& message)
{
	ssize_t size;
	const void* data;
	status_t result = message.FindData(fName, B_RAW_TYPE, &data, &size);
	if (result != B_OK)
		return result;

	if (size < 0)
		return B_ERROR;

	fFlatBuffer.SetSize(0);
	ssize_t written = fFlatBuffer.WriteAt(0, data, size);
	if (written != size) {
		fFlatBuffer.SetSize(0);
		return written < 0 ? written : B_ERROR;
	}

	return B_OK;
}


status_t
Keyring::WriteToMessage(BMessage& message)
{
	status_t result = _EncryptToFlatBuffer();
	if (result != B_OK)
		return result;

	return message.AddData(fName, B_RAW_TYPE, fFlatBuffer.Buffer(),
		fFlatBuffer.BufferLength());
}


status_t
Keyring::Access(const BMessage& keyMessage)
{
	fKeyMessage = keyMessage;

	status_t result = _DecryptFromFlatBuffer();
	if (result != B_OK) {
		fKeyMessage.MakeEmpty();
		return result;
	}

	fAccessible = true;
	return B_OK;
}


void
Keyring::RevokeAccess()
{
	if (!fAccessible)
		return;

	_EncryptToFlatBuffer();

	fKeyMessage.MakeEmpty();
	fData.MakeEmpty();
	fApplications.MakeEmpty();
	fAccessible = false;
}


bool
Keyring::IsAccessible() const
{
	return fAccessible;
}


const BMessage&
Keyring::KeyMessage() const
{
	return fKeyMessage;
}


status_t
Keyring::GetNextApplication(uint32& cookie, BString& signature,
	BString& path)
{
	char* nameFound = NULL;
	status_t result = fApplications.GetInfo(B_MESSAGE_TYPE, cookie++,
		&nameFound, NULL);
	if (result != B_OK)
		return B_ENTRY_NOT_FOUND;

	BMessage appMessage;
	result = fApplications.FindMessage(nameFound, &appMessage);
	if (result != B_OK)
		return B_ENTRY_NOT_FOUND;

	result = appMessage.FindString("path", &path);
	if (result != B_OK)
		return B_ERROR;

	signature = nameFound;
	return B_OK;
}


status_t
Keyring::FindApplication(const char* signature, const char* path,
	BMessage& appMessage)
{
	if (!fAccessible)
		return B_NOT_ALLOWED;

	int32 count;
	type_code type;
	if (fApplications.GetInfo(signature, &type, &count) != B_OK)
		return B_ENTRY_NOT_FOUND;

	for (int32 i = 0; i < count; i++) {
		if (fApplications.FindMessage(signature, i, &appMessage) != B_OK)
			continue;

		BString appPath;
		if (appMessage.FindString("path", &appPath) != B_OK)
			continue;

		if (appPath == path)
			return B_OK;
	}

	appMessage.MakeEmpty();
	return B_ENTRY_NOT_FOUND;
}


status_t
Keyring::AddApplication(const char* signature, const BMessage& appMessage)
{
	if (!fAccessible)
		return B_NOT_ALLOWED;

	status_t result = fApplications.AddMessage(signature, &appMessage);
	if (result != B_OK)
		return result;

	fModified = true;
	return B_OK;
}


status_t
Keyring::RemoveApplication(const char* signature, const char* path)
{
	if (!fAccessible)
		return B_NOT_ALLOWED;

	if (path == NULL) {
		// We want all of the entries for this signature removed.
		status_t result = fApplications.RemoveName(signature);
		if (result != B_OK)
			return B_ENTRY_NOT_FOUND;

		fModified = true;
		return B_OK;
	}

	int32 count;
	type_code type;
	if (fApplications.GetInfo(signature, &type, &count) != B_OK)
		return B_ENTRY_NOT_FOUND;

	for (int32 i = 0; i < count; i++) {
		BMessage appMessage;
		if (fApplications.FindMessage(signature, i, &appMessage) != B_OK)
			return B_ERROR;

		BString appPath;
		if (appMessage.FindString("path", &appPath) != B_OK)
			continue;

		if (appPath == path) {
			fApplications.RemoveData(signature, i);
			fModified = true;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
Keyring::FindKey(const BString& identifier, const BString& secondaryIdentifier,
	bool secondaryIdentifierOptional, BMessage* _foundKeyMessage) const
{
	if (!fAccessible)
		return B_NOT_ALLOWED;

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
	BMessage& _foundKeyMessage) const
{
	if (!fAccessible)
		return B_NOT_ALLOWED;

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
	if (!fAccessible)
		return B_NOT_ALLOWED;

	// Check for collisions.
	if (FindKey(identifier, secondaryIdentifier, false, NULL) == B_OK)
		return B_NAME_IN_USE;

	// We're fine, just add the new key.
	status_t result = fData.AddMessage(identifier, &keyMessage);
	if (result != B_OK)
		return result;

	fModified = true;
	return B_OK;
}


status_t
Keyring::RemoveKey(const BString& identifier,
	const BMessage& keyMessage)
{
	if (!fAccessible)
		return B_NOT_ALLOWED;

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

		status_t result = fData.RemoveData(identifier, i);
		if (result != B_OK)
			return result;

		fModified = true;
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


status_t
Keyring::_EncryptToFlatBuffer()
{
	if (!fModified)
		return B_OK;

	if (!fAccessible)
		return B_NOT_ALLOWED;

	BMessage container;
	status_t result = container.AddMessage("data", &fData);
	if (result != B_OK)
		return result;

	result = container.AddMessage("applications", &fApplications);
	if (result != B_OK)
		return result;

	fFlatBuffer.SetSize(0);
	fFlatBuffer.Seek(0, SEEK_SET);

	result = container.Flatten(&fFlatBuffer);
	if (result != B_OK)
		return result;

	// TODO: Actually encrypt the flat buffer...

	fModified = false;
	return B_OK;
}


status_t
Keyring::_DecryptFromFlatBuffer()
{
	if (fFlatBuffer.BufferLength() == 0)
		return B_OK;

	// TODO: Actually decrypt the flat buffer...

	BMessage container;
	fFlatBuffer.Seek(0, SEEK_SET);
	status_t result = container.Unflatten(&fFlatBuffer);
	if (result != B_OK)
		return result;

	result = container.FindMessage("data", &fData);
	if (result != B_OK)
		return result;

	result = container.FindMessage("applications", &fApplications);
	if (result != B_OK) {
		fData.MakeEmpty();
		return result;
	}

	return B_OK;
}
