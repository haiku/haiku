/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include <errno_private.h>


char *getlogin()
{
	struct passwd *pw;
	pw = getpwuid(getuid());
	if (pw)
		return pw->pw_name;
	__set_errno(ENOMEM);
	return NULL;
}


int getlogin_r(char *name, size_t nameSize)
{
	struct passwd *pw;
	pw = getpwuid(getuid());
	if (pw && (nameSize > 32/*PW_MAX_NAME*/)) {
		memset(name, 0, nameSize);
		strlcpy(name, pw->pw_name, 32/*PW_MAX_NAME*/);
		return B_OK;
	}
	return ENOMEM;
}


char *
cuserid(char *s)
{
	if (s != NULL && getlogin_r(s, L_cuserid))
		return s;

	return getlogin();
}

