/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_MAPPED_H
#define KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_MAPPED_H


#include <OS.h>

#include <cpu.h>
#include <kernel.h>
#include <smp.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/x86_physical_page_mapper.h"
#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"


struct kernel_args;


/*!	Physical page mapper implementation for use where the whole of physical
	memory is permanently mapped into the kernel address space.

	This is used on x86_64 where the virtual address space is likely a great
	deal larger than the amount of physical memory in the machine, so it can
	all be mapped in permanently, which is faster and makes life much easier.
*/


// #pragma mark - TranslationMapPhysicalPageMapper


inline void
TranslationMapPhysicalPageMapper::Delete()
{
	delete this;
}


inline void*
TranslationMapPhysicalPageMapper::GetPageTableAt(
	phys_addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	return (void*)(physicalAddress + KERNEL_PMAP_BASE);
}


// #pragma mark - X86PhysicalPageMapper


inline status_t
X86PhysicalPageMapper::CreateTranslationMapPhysicalPageMapper(
	TranslationMapPhysicalPageMapper** _mapper)
{
	auto mapper = new(std::nothrow) TranslationMapPhysicalPageMapper;
	if (mapper == NULL)
		return B_NO_MEMORY;

	*_mapper = mapper;
	return B_OK;
}


inline void*
X86PhysicalPageMapper::InterruptGetPageTableAt(
	phys_addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	return (void*)(physicalAddress + KERNEL_PMAP_BASE);
}


inline status_t
X86PhysicalPageMapper::GetPage(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	if (physicalAddress >= KERNEL_PMAP_BASE)
		return B_BAD_ADDRESS;

	*virtualAddress = physicalAddress + KERNEL_PMAP_BASE;
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::PutPage(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::GetPageCurrentCPU(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	if (physicalAddress >= KERNEL_PMAP_BASE)
		return B_BAD_ADDRESS;

	*virtualAddress = physicalAddress + KERNEL_PMAP_BASE;
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::PutPageCurrentCPU(addr_t virtualAddress,
	void* handle)
{
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::GetPageDebug(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	if (physicalAddress >= KERNEL_PMAP_BASE)
		return B_BAD_ADDRESS;

	*virtualAddress = physicalAddress + KERNEL_PMAP_BASE;
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::PutPageDebug(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::MemsetPhysical(phys_addr_t address, int value,
	phys_size_t length)
{
	if (address >= KERNEL_PMAP_SIZE || address + length > KERNEL_PMAP_SIZE)
		return B_BAD_ADDRESS;

	memset((void*)(address + KERNEL_PMAP_BASE), value, length);
	return B_OK;
}


inline status_t
X86PhysicalPageMapper::MemcpyFromPhysical(void* to, phys_addr_t _from,
	size_t length, bool user)
{
	if (_from >= KERNEL_PMAP_SIZE || _from + length > KERNEL_PMAP_SIZE)
		return B_BAD_ADDRESS;

	auto from = (void*)(_from + KERNEL_PMAP_BASE);

	if (user)
		return user_memcpy(to, from, length);
	else
		memcpy(to, from, length);

	return B_OK;
}


inline status_t
X86PhysicalPageMapper::MemcpyToPhysical(phys_addr_t _to, const void* from,
	size_t length, bool user)
{
	if (_to >= KERNEL_PMAP_SIZE || _to + length > KERNEL_PMAP_SIZE)
		return B_BAD_ADDRESS;

	auto to = (void*)(_to + KERNEL_PMAP_BASE);

	if (user)
		return user_memcpy(to, from, length);

	memcpy(to, from, length);
	return B_OK;
}


inline void
X86PhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to,
	phys_addr_t from)
{
	memcpy((void*)(to + KERNEL_PMAP_BASE), (void*)(from + KERNEL_PMAP_BASE),
		B_PAGE_SIZE);
}


status_t mapped_physical_page_ops_init(kernel_args* args,
	X86PhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper);


#endif	// KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_MAPPED_H
