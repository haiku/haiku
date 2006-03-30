/*
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <string.h>
#include <syscalls.h>


#define RLD_SCRATCH_SIZE 65536


static area_id rld_region;
static char *rld_base;
static char *rld_ptr;


void
rldheap_init(void)
{
	rld_region = _kern_create_area("rld scratch",
		(void **)&rld_base, B_ANY_ADDRESS, RLD_SCRATCH_SIZE,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	rld_ptr = rld_base;
}


void *
rldalloc(size_t s)
{
	void *retval;

	s = (s + 15) & ~15;
		// multiple of 16 bytes

	retval = rld_ptr;
	rld_ptr += s;

	return retval;
}


void
rldfree(void *p)
{
	(void)(p);
}

