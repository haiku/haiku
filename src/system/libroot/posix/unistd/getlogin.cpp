/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include <errno_private.h>
#include <user_group.h>


char*
getlogin()
{
	struct passwd *pw;
	pw = getpwuid(getuid());
	if (pw)
		return pw->pw_name;
	__set_errno(ENOMEM);
	return NULL;
}


int
getlogin_r(char *name, size_t nameSize)
{
	struct passwd* pw = NULL;
	struct passwd passwdBuffer;
	char passwdStringBuffer[MAX_PASSWD_BUFFER_SIZE];

	int status = getpwuid_r(getuid(), &passwdBuffer, passwdStringBuffer,
		sizeof(passwdStringBuffer), &pw);
	if (status != B_OK)
		return status;

	if (strnlen(pw->pw_name, LOGIN_NAME_MAX) >= nameSize)
		return ERANGE;

	memset(name, 0, nameSize);
	strlcpy(name, pw->pw_name, LOGIN_NAME_MAX);
	return B_OK;
}


char *
cuserid(char *s)
{
	if (s != NULL && getlogin_r(s, L_cuserid))
		return s;

	return getlogin();
}

