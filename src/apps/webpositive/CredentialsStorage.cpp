/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CredentialsStorage.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

#include "BrowserApp.h"


Credentials::Credentials()
	:
	fUsername(),
	fPassword()
{
}


Credentials::Credentials(const BString& username, const BString& password)
	:
	fUsername(username),
	fPassword(password)
{
}


Credentials::Credentials(const Credentials& other)
{
	*this = other;
}


Credentials::Credentials(const BMessage* archive)
{
	if (archive == NULL)
		return;
	archive->FindString("username", &fUsername);
	archive->FindString("password", &fPassword);
}


Credentials::~Credentials()
{
}


status_t
Credentials::Archive(BMessage* archive) const
{
	if (archive == NULL)
		return B_BAD_VALUE;
	status_t status = archive->AddString("username", fUsername);
	if (status == B_OK)
		status = archive->AddString("password", fPassword);
	return status;
}


Credentials&
Credentials::operator=(const Credentials& other)
{
	if (this == &other)
		return *this;

	fUsername = other.fUsername;
	fPassword = other.fPassword;

	return *this;
}


bool
Credentials::operator==(const Credentials& other) const
{
	if (this == &other)
		return true;

	return fUsername == other.fUsername && fPassword == other.fPassword;
}


bool
Credentials::operator!=(const Credentials& other) const
{
	return !(*this == other);
}


const BString&
Credentials::Username() const
{
	return fUsername;
}


const BString&
Credentials::Password() const
{
	return fPassword;
}


// #pragma mark - CredentialsStorage


CredentialsStorage
CredentialsStorage::sPersistentInstance(true);


CredentialsStorage
CredentialsStorage::sSessionInstance(false);


CredentialsStorage::CredentialsStorage(bool persistent)
	:
	BLocker(persistent ? "persistent credential storage"
		: "credential storage"),
	fCredentialMap(),
	fSettingsLoaded(false),
	fPersistent(persistent)
{
}


CredentialsStorage::~CredentialsStorage()
{
	_SaveSettings();
}


/*static*/ CredentialsStorage*
CredentialsStorage::SessionInstance()
{
	return &sSessionInstance;
}


/*static*/ CredentialsStorage*
CredentialsStorage::PersistentInstance()
{
	if (sPersistentInstance.Lock()) {
		sPersistentInstance._LoadSettings();
		sPersistentInstance.Unlock();
	}
	return &sPersistentInstance;
}


bool
CredentialsStorage::Contains(const HashKeyString& key)
{
	BAutolock _(this);

	return fCredentialMap.ContainsKey(key);
}


status_t
CredentialsStorage::PutCredentials(const HashKeyString& key,
	const Credentials& credentials)
{
	BAutolock _(this);

	return fCredentialMap.Put(key, credentials);
}


Credentials
CredentialsStorage::GetCredentials(const HashKeyString& key)
{
	BAutolock _(this);

	return fCredentialMap.Get(key);
}


// #pragma mark - private


void
CredentialsStorage::_LoadSettings()
{
	if (!fPersistent || fSettingsLoaded)
		return;

	fSettingsLoaded = true;

	BFile settingsFile;
	if (_OpenSettingsFile(settingsFile, B_READ_ONLY)) {
		BMessage settingsArchive;
		settingsArchive.Unflatten(&settingsFile);
		BMessage credentialsArchive;
		for (int32 i = 0; settingsArchive.FindMessage("credentials", i,
				&credentialsArchive) == B_OK; i++) {
			BString key;
			if (credentialsArchive.FindString("key", &key) == B_OK) {
				Credentials credentials(&credentialsArchive);
				fCredentialMap.Put(key, credentials);
			}
		}
	}
}


void
CredentialsStorage::_SaveSettings() const
{
	BFile settingsFile;
	if (_OpenSettingsFile(settingsFile,
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY)) {
		BMessage settingsArchive;
		BMessage credentialsArchive;
		CredentialMap::Iterator iterator = fCredentialMap.GetIterator();
		while (iterator.HasNext()) {
			const CredentialMap::Entry& entry = iterator.Next();
			if (entry.value.Archive(&credentialsArchive) != B_OK
				|| credentialsArchive.AddString("key",
					entry.key.value) != B_OK) {
				break;
			}
			if (settingsArchive.AddMessage("credentials",
					&credentialsArchive) != B_OK) {
				break;
			}
			credentialsArchive.MakeEmpty();
		}
		settingsArchive.Flatten(&settingsFile);
	}
}


bool
CredentialsStorage::_OpenSettingsFile(BFile& file, uint32 mode) const
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kApplicationName) != B_OK
		|| path.Append("CredentialsStorage") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}

