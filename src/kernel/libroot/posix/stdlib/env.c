/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <syscalls.h>
#include <stdlib.h>


int setenv(const char *name, const char *value, int overwrite)
{
	return sys_setenv(name, value, overwrite);
}


char *getenv(const char *name)
{
	char *value;
	int rc;
	
	rc = sys_getenv(name, &value);
	if (rc < 0)
		return NULL;
	return value;
}
