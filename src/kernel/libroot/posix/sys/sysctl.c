/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <sysctl.h>
#include <errno.h>
#include "syscalls.h"


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
sysctl(int *name, uint namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	/* ToDo: we should handle CTL_USER here, but as we don't even define it yet :) */
	int err = sys_sysctl(name, namelen, oldp, oldlenp, newp, newlen);
	
	RETURN_AND_SET_ERRNO(err)	
}

