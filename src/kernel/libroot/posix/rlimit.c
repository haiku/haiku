/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

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
	int status = _kern_getrlimit(resource, rlp);

	RETURN_AND_SET_ERRNO(status);
}


int
setrlimit(int resource, const struct rlimit *rlp)
{
	int status = _kern_setrlimit(resource, rlp);

	RETURN_AND_SET_ERRNO(status);
}
