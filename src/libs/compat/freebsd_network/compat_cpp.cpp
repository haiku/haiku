/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "compat_cpp.h"

#include <stdio.h>
#include <string.h>

#include <kernel/vm/vm.h>


void*
_kernel_contigmalloc_cpp(const char* file, int line, size_t size,
	phys_addr_t low, phys_addr_t high, phys_size_t alignment,
	phys_size_t boundary, bool zero, bool dontWait)
{
	size = ROUNDUP(size, B_PAGE_SIZE);

	uint32 creationFlags = (zero ? 0 : CREATE_AREA_DONT_CLEAR)
		| (dontWait ? CREATE_AREA_DONT_WAIT : 0);

	char name[B_OS_NAME_LENGTH];
	const char* baseName = strrchr(file, '/');
	baseName = baseName != NULL ? baseName + 1 : file;
	snprintf(name, sizeof(name), "contig:%s:%d", baseName, line);

	virtual_address_restrictions virtualRestrictions = {};

	physical_address_restrictions physicalRestrictions = {};
	physicalRestrictions.low_address = low;
	physicalRestrictions.high_address = high;
	physicalRestrictions.alignment = alignment;
	physicalRestrictions.boundary = boundary;

	void* address;
	area_id area = create_area_etc(B_SYSTEM_TEAM, name, size, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, creationFlags, 0,
		&virtualRestrictions, &physicalRestrictions, &address);
	if (area < 0)
		return NULL;

	return address;
}
