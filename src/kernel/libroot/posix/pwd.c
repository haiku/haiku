/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <pwd.h>
#include <syscalls.h>
#include <string.h>
#include <errno.h>


// ToDo: implement pwd stuff for real!


struct passwd *
getpwent(void)
{
	return NULL;
}


void
setpwent(void)
{
}


void
endpwent(void)
{
}


struct passwd *
getpwnam(const char *name)
{
	return NULL;
}


int
getpwnam_r(const char *name, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **result)
{
	return -1;
}


struct passwd *
getpwuid(uid_t uid)
{
	return NULL;
}


int
getpwuid_r(uid_t uid, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **result)
{
	return -1;
}
