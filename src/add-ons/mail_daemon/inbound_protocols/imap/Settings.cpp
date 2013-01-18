/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Settings.h"

#include <crypt.h>


Settings::Settings(const BMessage& archive)
	:
	fMessage(archive)
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
	BString server;
	if (fMessage.FindString("server", &server) == B_OK)
		return server;

	return "";
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
	int32 flavor;
	if (fMessage.FindInt32("flavor", &flavor) == B_OK)
		return flavor == 1;

	return false;
}


BString
Settings::Username() const
{
	BString username;
	if (fMessage.FindString("username", &username) == B_OK)
		return username;

	return "";
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
	BString destination;
	if (fMessage.FindString("destination", &destination) == B_OK)
		return BPath(destination);

	return "/boot/home/mail/in";
}
