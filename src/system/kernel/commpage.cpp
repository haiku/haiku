/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifdef COMMPAGE_COMPAT
#include <commpage_compat.h>
#else
#include <commpage.h>
#endif

#include <string.h>

#include <KernelExport.h>

#include <elf.h>
#include <vm/vm.h>
#include <vm/vm_types.h>

#ifndef ADDRESS_TYPE
#define ADDRESS_TYPE addr_t
#endif

static area_id	sCommPageArea;
static ADDRESS_TYPE*	sCommPageAddress;
static void*	sFreeCommPageSpace;
static image_id	sCommPageImage;


#define ALIGN_ENTRY(pointer)	(void*)ROUNDUP((addr_t)(pointer), 8)


void*
allocate_commpage_entry(int entry, size_t size)
{
	void* space = sFreeCommPageSpace;
	sFreeCommPageSpace = ALIGN_ENTRY((addr_t)sFreeCommPageSpace + size);
	sCommPageAddress[entry] = (addr_t)space - (addr_t)sCommPageAddress;
	dprintf("allocate_commpage_entry(%d, %lu) -> %p\n", entry, size,
		(void*)sCommPageAddress[entry]);
	return space;
}


addr_t
fill_commpage_entry(int entry, const void* copyFrom, size_t size)
{
	void* space = allocate_commpage_entry(entry, size);
	memcpy(space, copyFrom, size);
	return (addr_t)space - (addr_t)sCommPageAddress;
}


image_id
get_commpage_image()
{
	return sCommPageImage;
}


area_id
clone_commpage_area(team_id team, void** address)
{
	if (*address == NULL)
		*address = (void*)KERNEL_USER_DATA_BASE;
	return vm_clone_area(team, "commpage", address,
		B_RANDOMIZED_BASE_ADDRESS, B_READ_AREA | B_EXECUTE_AREA,
		REGION_PRIVATE_MAP, sCommPageArea, true);
}


status_t
commpage_init(void)
{
	// create a read/write kernel area
	sCommPageArea = create_area("kernel_commpage", (void **)&sCommPageAddress,
		B_ANY_ADDRESS, COMMPAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_WRITE_AREA | B_KERNEL_READ_AREA);

	// zero it out
	memset(sCommPageAddress, 0, COMMPAGE_SIZE);

	// fill in some of the table
	sCommPageAddress[0] = COMMPAGE_SIGNATURE;
	sCommPageAddress[1] = COMMPAGE_VERSION;

	// the next slot to allocate space is after the table
	sFreeCommPageSpace = ALIGN_ENTRY(&sCommPageAddress[COMMPAGE_TABLE_ENTRIES]);

	// create the image for the commpage
	sCommPageImage = elf_create_memory_image("commpage", 0, COMMPAGE_SIZE, 0,
		0);
	elf_add_memory_image_symbol(sCommPageImage, "commpage_table",
		0, COMMPAGE_TABLE_ENTRIES * sizeof(addr_t),
		B_SYMBOL_TYPE_DATA);

	arch_commpage_init();

	return B_OK;
}


status_t
commpage_init_post_cpus(void)
{
	return arch_commpage_init_post_cpus();
}
