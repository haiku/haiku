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


// ToDo: implement the user/group functions for real!


gid_t 
getegid(void)
{
	return 0;
}


uid_t 
geteuid(void)
{
	return 0;
}


gid_t 
getgid(void)
{
	return 0;
}


int 
getgroups(int groupSize, gid_t groupList[])
{
	return 0;
}


uid_t 
getuid(void)
{
	return 0;
}


char *
cuserid(char *s)
{
	if (s != NULL && getlogin_r(s, L_cuserid))
		return s;

	return getlogin();
}


int 
setgid(gid_t gid)
{
	if (gid == 0)
		return 0;

	return EPERM;
}


int 
setuid(uid_t uid)
{
	if (uid == 0)
		return 0;

	return EPERM;
}


int
setegid(gid_t gid)
{
	if (gid == 0)
		return 0;

	return EPERM;
}


int
seteuid(uid_t uid)
{
	if (uid == 0)
		return 0;

	return EPERM;
}


char *getlogin()
{
	struct passwd *pw;
	pw = getpwuid(getuid());
	if (pw)
		return pw->pw_name;
	errno = ENOMEM;
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


