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
#include <TLS.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>


struct net_settings;

extern "C" {
	int *_h_errnop(void);
	int _netstat(int fd, char **output, int verbose);
	int _socket_interrupt(void);
	int closesocket(int socket);
	char *find_net_setting(net_settings* settings, const char* heading,
		const char* name, char* value, unsigned numBytes);
	status_t set_net_setting(net_settings* settings, const char* heading,
		const char* name, const char* value);
	int getusername(char *user, size_t bufferLength);
	int getpassword(char *password, size_t bufferLength);
	char *_netconfig_find(const char *heaading, const char *name, char *value, int nbytes);
}


int32 __gHErrnoTLS = tls_allocate();


int *
_h_errnop(void)
{
	return (int *)tls_address(__gHErrnoTLS);
}


int
_netstat(int fd, char **output, int verbose)
{
	return ENOSYS;
}


int
_socket_interrupt(void)
{
	return -1;
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


int
getusername(char *user, size_t length)
{
	if (find_net_setting(NULL, "GLOBAL", "USERNAME", user, length) == NULL)
		return B_ERROR;

	return strlen(user);
}


int
getpassword(char *password, size_t length)
{
	if (find_net_setting(NULL, "GLOBAL", "PASSWORD", password, length) == NULL)
		return B_ERROR;

	return strlen(password);
}

char *
_netconfig_find(const char *heading, const char *name, char *value, int nbytes)
{
	return find_net_setting(NULL, heading, name, value, nbytes);
}
