/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Copyright 2004, Eli Green, eli@earthshattering.org. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
** All the functions in this file return the "baron" user.
** baron is uid 0, gid 0. the pw_name field is set to either
** "baron" or the value of the USER environment variable.
**
** This is done to be fully compatible with BeOS R5. /etc/passwd
** is not consulted.
**
** One difference:
** R5's getpwnam always returns NULL, but the implementation in this
** file returns passwd struct for baron if the name argument is set to
** the USER environment variable, or "baron" if USER is unset.
**
** The reentrant versions do not exist under BeOS R5.
*/


#include <pwd.h>
#include <stdlib.h>
#include <string.h>


static char _pw_first = 1;
static struct passwd _pw_struct;


static char *
get_user_name()
{
	char *user = getenv("USER");
	if (user != NULL)
		return user;

	return "baron";
}


static void
getbaron(struct passwd *pw)
{
	pw->pw_name = get_user_name();
	pw->pw_passwd = "";
	pw->pw_uid = 0;
	pw->pw_gid = 0;
	pw->pw_dir = "/boot/home";
	pw->pw_shell = "/bin/sh";
	pw->pw_gecos = "system";
}


struct passwd *
getpwent(void)
{
	if (!_pw_first)
		return NULL;

	_pw_first = 0;

	getbaron(&_pw_struct);
	return &_pw_struct;
}


void
setpwent(void)
{
	_pw_first = 1;
}


void
endpwent(void)
{
	_pw_first = 1;
}


struct passwd *
getpwnam(const char *name)
{
	if (!strcmp(name, get_user_name())) {
		getbaron(&_pw_struct);
		return &_pw_struct;
	}

	return NULL;
}


int
getpwnam_r(const char *name, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **_result)
{
	if (strcmp(name, get_user_name())) {
		*_result = NULL;
		return 0;
	}

	getbaron(passwd);
	*_result = passwd;
	return 0;
}


struct passwd *
getpwuid(uid_t uid)
{
	if (uid != 0)
		return NULL;

	getbaron(&_pw_struct);
	return &_pw_struct;
}


int
getpwuid_r(uid_t uid, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **_result)
{
	if (uid != 0) {
		*_result = NULL;
		return 0;
	}

	getbaron(passwd);
	*_result = passwd;
	return 0;
}
