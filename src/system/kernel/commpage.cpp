/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <commpage.h>

#include <string.h>

#include <KernelExport.h>

#include <vm.h>
#include <vm_types.h>


static area_id	sCommPageArea;
static area_id	sUserCommPageArea;
static addr_t*	sCommPageAddress;
static addr_t*	sUserCommPageAddress;
static void*	sFreeCommPageSpace;


#define ALIGN_ENTRY(pointer)	(void*)ROUNDUP((addr_t)(pointer), 8)


void*
allocate_commpage_entry(int entry, size_t size)
{
	void* space = sFreeCommPageSpace;
	sFreeCommPageSpace = ALIGN_ENTRY((addr_t)sFreeCommPageSpace + size);
	sCommPageAddress[entry] = (addr_t)sUserCommPageAddress
		+ ((addr_t)space - (addr_t)sCommPageAddress);
dprintf("allocate_commpage_entry(%d, %lu) -> %p\n", entry, size, (void*)sCommPageAddress[entry]);
	return space;
}


void*
fill_commpage_entry(int entry, const void* copyFrom, size_t size)
{
	void* space = allocate_commpage_entry(entry, size);
	memcpy(space, copyFrom, size);
	return space;
}


status_t
commpage_init(void)
{
	// create a read/write kernel area
	sCommPageArea = create_area("commpage", (void **)&sCommPageAddress,
		B_ANY_ADDRESS, COMMPAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_WRITE_AREA | B_KERNEL_READ_AREA);

	// clone it at a fixed address with user read/only permissions
	sUserCommPageAddress = (addr_t*)USER_COMMPAGE_ADDR;
	sUserCommPageArea = clone_area("user_commpage",
		(void **)&sUserCommPageAddress, B_EXACT_ADDRESS,
		B_READ_AREA | B_EXECUTE_AREA, sCommPageArea);

	// zero it out
	memset(sCommPageAddress, 0, COMMPAGE_SIZE);

	// fill in some of the table
	sCommPageAddress[0] = COMMPAGE_SIGNATURE;
	sCommPageAddress[1] = COMMPAGE_VERSION;

	// the next slot to allocate space is after the table
	sFreeCommPageSpace = ALIGN_ENTRY(&sCommPageAddress[COMMPAGE_TABLE_ENTRIES]);

	arch_commpage_init();

	return B_OK;
}

