/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <syscalls.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


char **environ = NULL;


int
setenv(const char *name, const char *value, int overwrite)
{
	status_t status = sys_setenv(name, value, overwrite);

	RETURN_AND_SET_ERRNO(status);
}


char *
getenv(const char *name)
{
	char *value;
	int rc;
	
	rc = sys_getenv(name, &value);
	if (rc < 0)
		return NULL;
	return value;
}


int
putenv(const char *string)
{
	char name[64];
	char *value = strchr(string, '=');

	if (value == NULL || value - string >= (int)sizeof(name)) {
		errno = EINVAL;
		return -1;
	}
	
	strlcpy(name, string, value - string);
	value++;

	return setenv(name, value, true);
}

