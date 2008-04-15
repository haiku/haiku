/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/mman.h>

#include <errno.h>

#include <OS.h>

#include <syscall_utils.h>
#include <syscalls.h>
#include <vm.h>


void*
mmap(void* address, size_t length, int protection, int flags, int fd,
	off_t offset)
{
	// offset and length must be page-aligned
	if (length == 0 || offset % B_PAGE_SIZE != 0) {
		errno = B_BAD_VALUE;
		return MAP_FAILED;
	}

	// check anonymous mapping
	if ((flags & MAP_ANONYMOUS) != 0) {
		fd = -1;
	} else if (fd < 0) {
		errno = EBADF;
		return MAP_FAILED;
	}

	// either MAP_SHARED or MAP_PRIVATE must be specified
	if (((flags & MAP_SHARED) != 0) == ((flags & MAP_PRIVATE) != 0)) {
		errno = B_BAD_VALUE;
		return MAP_FAILED;
	}

	// translate mapping, address specification, and protection
	int mapping = (flags & MAP_SHARED) != 0
		? REGION_NO_PRIVATE_MAP : REGION_PRIVATE_MAP;

	uint32 addressSpec = B_ANY_ADDRESS;
	if ((flags & MAP_FIXED) != 0)
		addressSpec = B_EXACT_ADDRESS;

	uint32 areaProtection = 0;
	if ((protection & PROT_READ) != 0)
		areaProtection |= B_READ_AREA;
	if ((protection & PROT_WRITE) != 0)
		areaProtection |= B_WRITE_AREA;
	if ((protection & PROT_EXEC) != 0)
		areaProtection |= B_EXECUTE_AREA;

	// ask the kernel to map
	area_id area = _kern_map_file("mmap area", &address, addressSpec,
		length, areaProtection, mapping, fd, offset);
	if (area < 0) {
		errno = area;
		return MAP_FAILED;
	}

	return address;
}


int
munmap(void* address, size_t length)
{
	RETURN_AND_SET_ERRNO(_kern_unmap_memory(address, length));
}
