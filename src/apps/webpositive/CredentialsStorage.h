/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CREDENTIAL_STORAGE_H
#define CREDENTIAL_STORAGE_H

#include "HashKeys.h"
#include "HashMap.h"
#include <Locker.h>
#include <String.h>


class BFile;
class BMessage;
class BString;


class Credentials {
public:
								Credentials();
								Credentials(const BString& username,
									const BString& password);
								Credentials(
									const Credentials& other);
								Credentials(const BMessage* archive);
								~Credentials();

			status_t			Archive(BMessage* archive) const;

			Credentials&		operator=(const Credentials& other);

			bool				operator==(const Credentials& other) const;
			bool				operator!=(const Credentials& other) const;

			const BString&		Username() const;
			const BString&		Password() const;

private:
			BString				fUsername;
			BString				fPassword;
};


class CredentialsStorage : public BLocker {
public:
	static	CredentialsStorage*	SessionInstance();
	static	CredentialsStorage*	PersistentInstance();

			bool				Contains(const HashKeyString& key);
			status_t			PutCredentials(const HashKeyString& key,
									const Credentials& credentials);
			Credentials			GetCredentials(const HashKeyString& key);

private:
								CredentialsStorage(bool persistent);
	virtual						~CredentialsStorage();

			void				_LoadSettings();
			void				_SaveSettings() const;
			bool				_OpenSettingsFile(BFile& file,
									uint32 mode) const;

private:
			typedef HashMap<HashKeyString, Credentials> CredentialMap;
			CredentialMap		fCredentialMap;

	static	CredentialsStorage	sPersistentInstance;
	static	CredentialsStorage	sSessionInstance;
			bool				fSettingsLoaded;
			bool				fPersistent;
};


#endif // CREDENTIAL_STORAGE_H

