/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Settings.h"

#include <crypt.h>


Settings::Settings(const char* accountName, const BMessage& archive)
	:
	fMessage(archive),
	fAccountName(accountName)
{
}


Settings::~Settings()
{
}


BNetworkAddress
Settings::ServerAddress() const
{
	return BNetworkAddress(Server(), Port());
}


BString
Settings::Server() const
{
	return fMessage.GetString("server", "");
}


uint16
Settings::Port() const
{
	int32 port;
	if (fMessage.FindInt32("port", &port) == B_OK)
		return port;

	return UseSSL() ? 993 : 143;
}


bool
Settings::UseSSL() const
{
	return fMessage.GetInt32("flavor", 1) == 1;
}


BString
Settings::Username() const
{
	return fMessage.GetString("username", "");
}


BString
Settings::Password() const
{
	BString password;
	char* passwd = get_passwd(&fMessage, "cpasswd");
	if (passwd != NULL) {
		password = passwd;
		delete[] passwd;
		return password;
	}

	return "";
}


BPath
Settings::Destination() const
{
	BPath path(fMessage.FindString("destination"));
	if (path.Path() == NULL) {
		// Use default directory
		path = "/boot/home/mail";
		path.Append(fAccountName.String());
	}
	return path;
}


int32
Settings::MaxConnections() const
{
	return fMessage.GetInt32("max connections", 1);
}


bool
Settings::IdleMode() const
{
	return fMessage.GetBool("idle", true);
}


int32
Settings::BodyFetchLimit() const
{
	return fMessage.GetInt32("partial_download_limit", -1);
}

bool
Settings::DeleteRemoteWhenLocal() const
{
	return fMessage.FindBool("delete_remote_when_local");
}
