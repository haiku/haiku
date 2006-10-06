/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Jérôme Duval, korli@users.berlios.de
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <SupportDefs.h>

#include <string.h>
#include <unistd.h>


struct net_settings;

int _netstat(int fd, char **output, int verbose);
int closesocket(int socket);
char *find_net_setting(net_settings* settings, const char* heading,
	const char* name, char* value, unsigned numBytes);
status_t set_net_setting(net_settings* settings, const char* heading,
	const char* name, const char* value);


int
_netstat(int fd, char **output, int verbose)
{
	return ENOSYS;
}


int
closesocket(int socket)
{
	return close(socket);
}


/* TODO: This is a terrible hack :(
 * TODO: We should really get these settings values by parsing the real settings
 */

char *
find_net_setting(net_settings* settings, const char* heading,
	const char* name, char* value, unsigned numBytes)
{
	if (strcmp(heading, "GLOBAL") != 0)
		return NULL;

	if (!strcmp(name, "HOSTNAME"))
		strlcpy(value, "hostname", numBytes);
	else if (!strcmp(name, "USERNAME"))
		strlcpy(value, "baron", numBytes);
	else if (!strcmp(name, "PASSWORD"))
		strlcpy(value, "password", numBytes);
	else if (!strcmp(name, "FTP_ENABLED"))
		strlcpy(value, "1", numBytes);
	else if (!strcmp(name, "TELNETD_ENABLED"))
		strlcpy(value, "1", numBytes);
	else
		return NULL;
	
	return value;
}


status_t
set_net_setting(net_settings* settings, const char* heading,
	const char* name, const char* value)
{
	return B_UNSUPPORTED;
}

