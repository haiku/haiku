/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */

#include <Errors.h>
#include <OS.h>
#include <string.h>

#include "debug.h"
#include "util.h"

static spinlock slock = B_SPINLOCK_INITIALIZER;
static uint32 round_to_pagesize(uint32 size);

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
alloc_mem(physical_entry *phy, addr_t *log, size_t size, const char *name)
{
	void * logadr;
	area_id areaid;
	status_t rv;

	ITRACE("Allocating %s: ", name);

	size = round_to_pagesize(size);
	areaid = create_area(name, &logadr, B_ANY_KERNEL_ADDRESS, size,
		B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
	if (areaid < B_OK) {
		ITRACE("couldn't allocate\n");
		return B_ERROR;
	}
	rv = get_memory_map(logadr, size, phy, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		ITRACE("couldn't map\n");
		return B_ERROR;
	}

	if (log)
		*log = (addr_t)logadr;

	ITRACE("area = %" B_PRId32 ", size = %" B_PRIuSIZE ", log = 0x%" \
		B_PRIXADDR ", phy = 0x%" B_PRIXPHYSADDR "\n", areaid, size,
		*log, phy->address);

	return areaid;
}

