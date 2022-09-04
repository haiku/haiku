/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include "PMAPPhysicalPageMapper.h"


status_t
PMAPPhysicalPageMapper::GetPage(
	phys_addr_t physicalAddress, addr_t* _virtualAddress, void** _handle)
{
	ASSERT(physicalAddress < KERNEL_PMAP_SIZE);

	*_virtualAddress = KERNEL_PMAP_BASE + physicalAddress;
	*_handle = NULL;

	return B_OK;
}


status_t
PMAPPhysicalPageMapper::PutPage(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


status_t
PMAPPhysicalPageMapper::MemsetPhysical(phys_addr_t address, int value, phys_size_t length)
{
	ASSERT(address < KERNEL_PMAP_SIZE);
	memset(reinterpret_cast<void*>(KERNEL_PMAP_BASE + address), value, length);

	return B_OK;
}


status_t
PMAPPhysicalPageMapper::MemcpyFromPhysical(void* to, phys_addr_t from, size_t length, bool user)
{
	if (user)
		panic("MemcpyFromPhysical user not impl");

	ASSERT(from < KERNEL_PMAP_SIZE);
	memcpy(to, reinterpret_cast<void*>(KERNEL_PMAP_BASE + from), length);

	return B_OK;
}


status_t
PMAPPhysicalPageMapper::MemcpyToPhysical(phys_addr_t to, const void* from, size_t length, bool user)
{
	if (user)
		panic("MemcpyToPhysical user not impl");

	ASSERT(to < KERNEL_PMAP_SIZE);
	memcpy(reinterpret_cast<void*>(KERNEL_PMAP_BASE + to), from, length);

	return B_OK;
}


void
PMAPPhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to, phys_addr_t from)
{
	ASSERT(to < KERNEL_PMAP_SIZE);
	ASSERT(from < KERNEL_PMAP_SIZE);
	memcpy(reinterpret_cast<void*>(KERNEL_PMAP_BASE + to),
		reinterpret_cast<void*>(KERNEL_PMAP_BASE + from), B_PAGE_SIZE);
}
