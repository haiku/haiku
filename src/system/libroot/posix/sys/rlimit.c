/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <sys/resource.h>
#include <syscalls.h>
#include <errno.h>

#include <errno_private.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		__set_errno(err); \
		return -1; \
	} \
	return err;


int
getrlimit(int resource, struct rlimit *rlimit)
{
	int status = _kern_getrlimit(resource, rlimit);

	RETURN_AND_SET_ERRNO(status);
}


int
setrlimit(int resource, const struct rlimit *rlimit)
{
	int status = _kern_setrlimit(resource, rlimit);

	RETURN_AND_SET_ERRNO(status);
}

