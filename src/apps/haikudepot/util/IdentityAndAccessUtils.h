/*
 * Copyright 2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef IDENTITY_AND_ACCESS_UTILS_H
#define IDENTITY_AND_ACCESS_UTILS_H

#include <set>

#include <String.h>

#include "UserCredentials.h"


class IdentityAndAccessUtils {

public:
	static	status_t		ClearCredentials();
	static	status_t		StoreCredentials(const UserCredentials& credentials);
	static	status_t		RetrieveCredentials(const BString& nickname,
								UserCredentials& credentials);

private:
	static	status_t		_CollectStoredIdentifiers(std::set<BString>& identifiers);
	static	status_t		_RemoveKeyForIdentifier(const BString& identifier);

	static	BString			_ToIdentifier(const BString& nickname);
};

#endif // IDENTITY_AND_ACCESS_UTILS_H