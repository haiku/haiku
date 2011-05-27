/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#include <Errors.h>
#include <OS.h>
#include <string.h>

#include "debug.h"
#include "util.h"

spinlock slock = 0;

uint32 round_to_pagesize(uint32 size);


cpu_status
lock(void)
{
	cpu_status status = disable_interrupts();
	acquire_spinlock(&slock);
	return status;
}


void
unlock(cpu_status status)
{
	release_spinlock(&slock);
	restore_interrupts(status);
}


uint32
round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}


area_id
alloc_mem(void **phy, void **log, size_t size, const char *name)
{
// TODO: phy should be phys_addr_t*!
	physical_entry pe;
	void * logadr;
	area_id areaid;
	status_t rv;

	TRACE("allocating %#08X bytes for %s\n", (int)size, name);

	size = round_to_pagesize(size);
	areaid = create_area(name, &logadr, B_ANY_KERNEL_ADDRESS, size,
		B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
	if (areaid < B_OK) {
		TRACE("couldn't allocate area %s\n",name);
		return B_ERROR;
	}
	rv = get_memory_map(logadr,size,&pe,1);
	if (rv < B_OK) {
		delete_area(areaid);
		TRACE("couldn't map %s\n",name);
		return B_ERROR;
	}
	memset(logadr,0,size);
	if (log)
		*log = logadr;
	if (phy)
		*phy = (void*)(addr_t)pe.address;
	TRACE("area = %d, size = %#08X, log = %#08X, phy = %#08X\n",
        (int)areaid, (int)size, (int)logadr, (int)pe.address);
	return areaid;
}

