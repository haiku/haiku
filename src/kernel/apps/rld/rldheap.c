/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <string.h>
#include <syscalls.h>
#include "rld_priv.h"


#define RLD_SCRATCH_SIZE 65536
#define RLD_PROGRAM_BASE 0x00200000	/* keep in sync with app ldscript */


static region_id rld_region;
static region_id rld_region_2;
static char *rld_base;
static char *rld_base_2;
static char *rld_ptr;


void
rldheap_init(void)
{
	rld_region = _kern_create_area("rld scratch",
		(void **)&rld_base, B_ANY_ADDRESS, RLD_SCRATCH_SIZE,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	/*
	 * Fill in the gap upto RLD_PROGRAM_BASE,
	 * 
	 * NOTE: this will be required only untile the vm finally
	 * support REGION_ADDR_ANY_ABOVE and REGION_ADDR_ANY_BELOW.
	 * Not doing these leads to some funny troubles with some
	 * libraries.
	 */
	rld_region_2 = _kern_create_area("RLD_padding", (void **)&rld_base_2,
		B_ANY_ADDRESS, RLD_PROGRAM_BASE - (uint32)(rld_base + RLD_SCRATCH_SIZE),
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

