/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Keyring.h"


Keyring::Keyring()
	:
	fHasUnlockKey(false),
	fUnlocked(false),
	fModified(false)
{
}


Keyring::Keyring(const char* name)
	:
	fName(name),
	fHasUnlockKey(false),
	fUnlocked(false),
	fModified(false)
{
}


Keyring::~Keyring()
{
}


status_t
Keyring::ReadFromMessage(const BMessage& message)
{
	status_t result = message.FindString("name", &fName);
	if (result != B_OK)
		return result;

	result = message.FindBool("hasUnlockKey", &fHasUnlockKey);
	if (result != B_OK)
		return result;

	if (message.GetBool("noData", false)) {
		fFlatBuffer.SetSize(0);
		return B_OK;
	}

	ssize_t size;
	const void* data;
	result = message.FindData("data", B_RAW_TYPE, &data, &size);
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

	if (fFlatBuffer.BufferLength() == 0)
		result = message.AddBool("noData", true);
	else {
		result = message.AddData("data", B_RAW_TYPE, fFlatBuffer.Buffer(),
			fFlatBuffer.BufferLength());
	}
	if (result != B_OK)
		return result;

	result = message.AddBool("hasUnlockKey", fHasUnlockKey);
	if (result != B_OK)
		return result;

	return message.AddString("name", fName);
}


status_t
Keyring::Unlock(const BMessage* keyMessage)
{
	if (fUnlocked)
		return B_OK;

	if (fHasUnlockKey == (keyMessage == NULL))
		return B_BAD_VALUE;

	if (keyMessage != NULL)
		fUnlockKey = *keyMessage;

	status_t result = _DecryptFromFlatBuffer();
	if (result != B_OK) {
		fUnlockKey.MakeEmpty();
		return result;
	}

	fUnlocked = true;
	return B_OK;
}


void
Keyring::Lock()
{
	if (!fUnlocked)
		return;

	_EncryptToFlatBuffer();

	fUnlockKey.MakeEmpty();
	fData.MakeEmpty();
	fApplications.MakeEmpty();
	fUnlocked = false;
}


bool
Keyring::IsUnlocked() const
{
	return fUnlocked;
}


bool
Keyring::HasUnlockKey() const
{
	return fHasUnlockKey;
}


const BMessage&
Keyring::UnlockKey() const
{
	return fUnlockKey;
}


status_t
Keyring::SetUnlockKey(const BMessage& keyMessage)
{
	if (!fUnlocked)
		return B_NOT_ALLOWED;

	fHasUnlockKey = true;
	fUnlockKey = keyMessage;
	fModified = true;
	return B_OK;
}


status_t
Keyring::RemoveUnlockKey()
{
	if (!fUnlocked)
		return B_NOT_ALLOWED;

	fUnlockKey.MakeEmpty();
	fHasUnlockKey = false;
	fModified = true;
	return B_OK;
}


status_t
Keyring::GetNextApplication(uint32& cookie, BString& signature,
	BString& path)
{
	if (!fUnlocked)
		return B_NOT_ALLOWED;

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
	if (!fUnlocked)
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
	if (!fUnlocked)
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
	if (!fUnlocked)
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
	if (!fUnlocked)
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
	if (!fUnlocked)
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
	if (!fUnlocked)
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
	if (!fUnlocked)
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

	if (!fUnlocked)
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

	if (fHasUnlockKey) {
		// TODO: Actually encrypt the flat buffer...
	}

	fModified = false;
	return B_OK;
}


status_t
Keyring::_DecryptFromFlatBuffer()
{
	if (fFlatBuffer.BufferLength() == 0)
		return B_OK;

	if (fHasUnlockKey) {
		// TODO: Actually decrypt the flat buffer...
	}

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
