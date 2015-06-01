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

#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "debug.h"
#include "util.h"

static spinlock slock = B_SPINLOCK_INITIALIZER;

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


area_id
alloc_mem(physical_entry *phy, addr_t *log, size_t size, const char *name)
{
	void * logadr;
	area_id areaid;
	status_t rv;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	physicalRestrictions.high_address = 1 << 28;
	// ICE1712 chipset can not deal with memory area beyond 256MB

	ITRACE("Allocating %s: ", name);

	areaid = vm_create_anonymous_area(B_SYSTEM_TEAM, name, size, B_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA, 0, 0,
		&virtualRestrictions, &physicalRestrictions, true, &logadr);

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

