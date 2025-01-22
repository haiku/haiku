/*
 * Copyright 2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "IdentityAndAccessUtils.h"

#include <set>

#include <KeyStore.h>

#include "Logger.h"


static const char* kHaikuDepotKeyring = "HaikuDepot";

static const char* kKeyIdentifierPrefix = "hds.password.";
	// this prefix is added before the nickname in the keystore
	// so that HDS username/password pairs can be identified.


/*static*/ status_t
IdentityAndAccessUtils::ClearCredentials()
{
	std::set<BString> identifiersToDelete;
	status_t result = B_OK;

	if (result == B_OK)
		result = _CollectStoredIdentifiers(identifiersToDelete);

	if (result == B_OK) {
		HDINFO("found %" B_PRIu32 " stored passwords to clear",
			static_cast<uint32>(identifiersToDelete.size()));
	} else {
		HDERROR("unable to get the stored passwords to clear");
	}

	if (result == B_OK) {
		std::set<BString>::const_iterator it;
		for (it = identifiersToDelete.begin(); it != identifiersToDelete.end() && result == B_OK;
			it++) {
			const BString& identifierToDelete = *it;
			result = _RemoveKeyForIdentifier(identifierToDelete);

			if (result == B_OK)
				HDINFO("cleared password [%s]", identifierToDelete.String());
			else
				HDINFO("failed to clear password [%s]", identifierToDelete.String());
		}
	}

	return result;
}


/*static*/ status_t
IdentityAndAccessUtils::StoreCredentials(const UserCredentials& credentials)
{
	if (credentials.Nickname().IsEmpty())
		return B_BAD_DATA;

	BString identifier = _ToIdentifier(credentials.Nickname());

	if (!credentials.PasswordClear())
		return _RemoveKeyForIdentifier(identifier);

	status_t result = B_OK;
	BPasswordKey key(credentials.PasswordClear(), B_KEY_PURPOSE_WEB, identifier);
	BKeyStore keyStore;

	if (result == B_OK) {
		result = keyStore.AddKeyring(kHaikuDepotKeyring);

		switch (result) {
			case B_OK:
				HDINFO("did create keyring [%s]", kHaikuDepotKeyring);
				break;
			case B_NAME_IN_USE:
				HDTRACE("keyring [%s] already exists", kHaikuDepotKeyring);
				result = B_OK;
				break;
			default:
				break;
		}
	}

	if (result == B_OK) {
		result = keyStore.AddKey(kHaikuDepotKeyring, key);

		switch (result) {
			case B_OK:
				HDINFO("did store the key [%s] in keyring [%s]", identifier.String(),
					kHaikuDepotKeyring);
				break;
			case B_BAD_VALUE:
				HDERROR("keyring [%s] does not exist", kHaikuDepotKeyring);
				break;
			case B_NAME_IN_USE:
				HDERROR("the key [%s] is already in use in keyring [%s]", identifier.String(),
					kHaikuDepotKeyring);
				break;
			case B_NOT_ALLOWED:
				HDERROR("it was disallowed to store the key [%s] in keyring [%s]",
					identifier.String(), kHaikuDepotKeyring);
				break;
			default:
				HDERROR("an unknown error occurred storing the key [%s] in keyring [%s]",
					identifier.String(), kHaikuDepotKeyring);
				break;
		}
	}

	return result;
}

/*!	This method will obtain the password if it is present and store it into the
	credentials supplied. On entry, the credentials should at least contain the
	nickname.
*/
/*static*/ status_t
IdentityAndAccessUtils::RetrieveCredentials(const BString& nickname, UserCredentials& credentials)
{
	if (nickname.IsEmpty())
		return B_BAD_DATA;

	BPasswordKey key;
	BKeyStore keyStore;
	BString identifier = _ToIdentifier(nickname);
	status_t result;

	result = keyStore.GetKey(kHaikuDepotKeyring, B_KEY_TYPE_PASSWORD, identifier, key);

	if (result == B_OK) {
		BString passwordClear = key.Password();

		if (passwordClear != credentials.PasswordClear()) {
			credentials.SetNickname(nickname);
			credentials.SetPasswordClear(passwordClear);
			credentials.SetIsSuccessful(false);
		}
	}

	return result;
}


/*static*/ status_t
IdentityAndAccessUtils::_CollectStoredIdentifiers(std::set<BString>& identifiers)
{
	uint32 cookie = 0;
	size_t prefixLen = strlen(kKeyIdentifierPrefix);
	BKeyStore keyStore;

	while (true) {
		BPasswordKey password;
		status_t result = keyStore.GetNextKey(kHaikuDepotKeyring, B_KEY_TYPE_PASSWORD,
			B_KEY_PURPOSE_ANY, cookie, password);

		switch (result) {
			case B_ENTRY_NOT_FOUND: // last item
				return B_OK;
			case B_BAD_VALUE: // no key ring found
				return B_OK;
			case B_OK:
			{
				const char* passwordIdentifier = password.Identifier();
				if (strncmp(kKeyIdentifierPrefix, passwordIdentifier, prefixLen) == 0)
					identifiers.insert(BString(passwordIdentifier));
				break;
			}
			default:
				return result;
		}
	}
}


/*static*/ status_t
IdentityAndAccessUtils::_RemoveKeyForIdentifier(const BString& identifier)
{
	BKeyStore keyStore;
	BPasswordKey key;
	status_t result = keyStore.GetKey(kHaikuDepotKeyring, B_KEY_TYPE_PASSWORD, identifier, key);

	switch (result) {
		case B_OK:
			result = keyStore.RemoveKey(kHaikuDepotKeyring, key);
			if (result != B_OK) {
				HDERROR("error occurred when removing password for [%s] : %s", identifier.String(),
					strerror(result));
			}
			return B_OK;
		case B_ENTRY_NOT_FOUND:
			return B_OK;
		default:
			HDERROR("error occurred when finding password for [%s]: %s", identifier.String(),
				strerror(result));
			return result;
	}
}


/*static*/ BString
IdentityAndAccessUtils::_ToIdentifier(const BString& nickname)
{
	if (nickname.IsEmpty())
		return "";
	return BString(kKeyIdentifierPrefix) << nickname;
}
