/*
 * Copyright 2008-2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "generic_vm_physical_page_ops.h"

#include <vm/vm.h>
#include <util/AutoLock.h>


status_t
generic_vm_memset_physical(phys_addr_t address, int value, phys_size_t length)
{
	ThreadCPUPinner _(thread_get_current_thread());

	while (length > 0) {
		phys_addr_t pageOffset = address % B_PAGE_SIZE;
		addr_t virtualAddress;
		void* handle;
		status_t error = vm_get_physical_page_current_cpu(address - pageOffset,
			&virtualAddress, &handle);
		if (error != B_OK)
			return error;

		size_t toSet = min_c(length, B_PAGE_SIZE - pageOffset);
		memset((void*)(virtualAddress + pageOffset), value, toSet);

		vm_put_physical_page_current_cpu(virtualAddress, handle);

		length -= toSet;
		address += toSet;
	}

	return B_OK;
}


status_t
generic_vm_memcpy_from_physical(void* _to, phys_addr_t from, size_t length,
	bool user)
{
	uint8* to = (uint8*)_to;
	phys_addr_t pageOffset = from % B_PAGE_SIZE;

	ThreadCPUPinner _(thread_get_current_thread());

	while (length > 0) {
		size_t toCopy = min_c(length, B_PAGE_SIZE - pageOffset);

		addr_t virtualAddress;
		void* handle;
		status_t error = vm_get_physical_page_current_cpu(from - pageOffset,
			&virtualAddress, &handle);
		if (error != B_OK)
			return error;

		if (user) {
			error = user_memcpy(to, (void*)(virtualAddress + pageOffset),
				toCopy);
		} else
			memcpy(to, (void*)(virtualAddress + pageOffset), toCopy);

		vm_put_physical_page_current_cpu(virtualAddress, handle);

		if (error != B_OK)
			return error;

		to += toCopy;
		from += toCopy;
		length -= toCopy;
		pageOffset = 0;
	}

	return B_OK;
}


status_t
generic_vm_memcpy_to_physical(phys_addr_t to, const void* _from, size_t length,
	bool user)
{
	const uint8* from = (const uint8*)_from;
	phys_addr_t pageOffset = to % B_PAGE_SIZE;

	ThreadCPUPinner _(thread_get_current_thread());

	while (length > 0) {
		size_t toCopy = min_c(length, B_PAGE_SIZE - pageOffset);

		addr_t virtualAddress;
		void* handle;
		status_t error = vm_get_physical_page_current_cpu(to - pageOffset,
			&virtualAddress, &handle);
		if (error != B_OK)
			return error;

		if (user) {
			error = user_memcpy((void*)(virtualAddress + pageOffset), from,
				toCopy);
		} else
			memcpy((void*)(virtualAddress + pageOffset), from, toCopy);

		vm_put_physical_page_current_cpu(virtualAddress, handle);

		if (error != B_OK)
			return error;

		to += toCopy;
		from += toCopy;
		length -= toCopy;
		pageOffset = 0;
	}

	return B_OK;
}


/*!	NOTE: If this function is used, vm_get_physical_page_current_cpu() must not
	be blocking, since we need to call it twice and could thus deadlock.
*/
void
generic_vm_memcpy_physical_page(phys_addr_t to, phys_addr_t from)
{
	ThreadCPUPinner _(thread_get_current_thread());

	// map source page
	addr_t fromVirtual;
	void* fromHandle;
	status_t error = vm_get_physical_page_current_cpu(from, &fromVirtual,
		&fromHandle);
	if (error != B_OK) {
		panic("generic_vm_memcpy_physical_page(): Failed to map source page!");
		return;
	}

	// map destination page
	addr_t toVirtual;
	void* toHandle;
	error = vm_get_physical_page_current_cpu(to, &toVirtual, &toHandle);
	if (error == B_OK) {
		// both pages are mapped -- copy
		memcpy((void*)toVirtual, (const void*)fromVirtual, B_PAGE_SIZE);
		vm_put_physical_page_current_cpu(toVirtual, toHandle);
	} else {
		panic("generic_vm_memcpy_physical_page(): Failed to map destination "
			"page!");
	}

	vm_put_physical_page_current_cpu(fromVirtual, fromHandle);
}
