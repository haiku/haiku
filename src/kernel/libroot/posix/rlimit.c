/*
** Copyright 2002, Jeff Hamilton. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <sys/resource.h>
#include <syscalls.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
getrlimit(int resource, struct rlimit *rlp)
{
	int status = sys_getrlimit(resource, rlp);

	RETURN_AND_SET_ERRNO(status);
}


int
setrlimit(int resource, const struct rlimit *rlp)
{
	int status = sys_setrlimit(resource, rlp);

	RETURN_AND_SET_ERRNO(status);
}
