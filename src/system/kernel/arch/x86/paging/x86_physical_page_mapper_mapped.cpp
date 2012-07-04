/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


/*!	Physical page mapper implementation for use where the whole of physical
	memory is permanently mapped into the kernel address space.

	This is used on x86_64 where the virtual address space is likely a great
	deal larger than the amount of physical memory in the machine, so it can
	all be mapped in permanently, which is faster and makes life much easier.
*/


#include "paging/x86_physical_page_mapper_mapped.h"

#include <new>

#include <cpu.h>
#include <kernel.h>
#include <smp.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/x86_physical_page_mapper.h"
#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"


// #pragma mark -


class MappedTranslationMapPhysicalPageMapper
	: public TranslationMapPhysicalPageMapper {
public:
	virtual	void				Delete();

	virtual	void*				GetPageTableAt(phys_addr_t physicalAddress);
};


class MappedPhysicalPageMapper : public X86PhysicalPageMapper {
public:
	virtual	status_t			CreateTranslationMapPhysicalPageMapper(
									TranslationMapPhysicalPageMapper** _mapper);

	virtual	void*				InterruptGetPageTableAt(
									phys_addr_t physicalAddress);

	virtual	status_t			GetPage(phys_addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	virtual	status_t			PutPage(addr_t virtualAddress, void* handle);

	virtual	status_t			GetPageCurrentCPU(phys_addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	virtual	status_t			PutPageCurrentCPU(addr_t virtualAddress,
									void* handle);

	virtual	status_t			GetPageDebug(phys_addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	virtual	status_t			PutPageDebug(addr_t virtualAddress,
									void* handle);

	virtual	status_t			MemsetPhysical(phys_addr_t address, int value,
									phys_size_t length);
	virtual	status_t			MemcpyFromPhysical(void* to, phys_addr_t from,
									size_t length, bool user);
	virtual	status_t			MemcpyToPhysical(phys_addr_t to,
									const void* from, size_t length, bool user);
	virtual	void				MemcpyPhysicalPage(phys_addr_t to,
									phys_addr_t from);
};


static MappedPhysicalPageMapper sPhysicalPageMapper;
static MappedTranslationMapPhysicalPageMapper sKernelPageMapper;


// #pragma mark - MappedTranslationMapPhysicalPageMapper


void
MappedTranslationMapPhysicalPageMapper::Delete()
{
	delete this;
}


void*
MappedTranslationMapPhysicalPageMapper::GetPageTableAt(
	phys_addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	return (void*)(physicalAddress + KERNEL_PMAP_BASE);
}


// #pragma mark - MappedPhysicalPageMapper


status_t
MappedPhysicalPageMapper::CreateTranslationMapPhysicalPageMapper(
	TranslationMapPhysicalPageMapper** _mapper)
{
	MappedTranslationMapPhysicalPageMapper* mapper
		= new(std::nothrow) MappedTranslationMapPhysicalPageMapper;
	if (mapper == NULL)
		return B_NO_MEMORY;

	*_mapper = mapper;
	return B_OK;
}


void*
MappedPhysicalPageMapper::InterruptGetPageTableAt(
	phys_addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	return (void*)(physicalAddress + KERNEL_PMAP_BASE);
}


status_t
MappedPhysicalPageMapper::GetPage(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	if (physicalAddress >= KERNEL_PMAP_BASE)
		return B_BAD_ADDRESS;

	*virtualAddress = physicalAddress + KERNEL_PMAP_BASE;
	return B_OK;
}


status_t
MappedPhysicalPageMapper::PutPage(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


status_t
MappedPhysicalPageMapper::GetPageCurrentCPU(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	if (physicalAddress >= KERNEL_PMAP_BASE)
		return B_BAD_ADDRESS;

	*virtualAddress = physicalAddress + KERNEL_PMAP_BASE;
	return B_OK;
}


status_t
MappedPhysicalPageMapper::PutPageCurrentCPU(addr_t virtualAddress,
	void* handle)
{
	return B_OK;
}


status_t
MappedPhysicalPageMapper::GetPageDebug(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	if (physicalAddress >= KERNEL_PMAP_BASE)
		return B_BAD_ADDRESS;

	*virtualAddress = physicalAddress + KERNEL_PMAP_BASE;
	return B_OK;
}


status_t
MappedPhysicalPageMapper::PutPageDebug(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


status_t
MappedPhysicalPageMapper::MemsetPhysical(phys_addr_t address, int value,
	phys_size_t length)
{
	if (address >= KERNEL_PMAP_SIZE || address + length > KERNEL_PMAP_SIZE)
		return B_BAD_ADDRESS;

	memset((void*)(address + KERNEL_PMAP_BASE), value, length);
	return B_OK;
}


status_t
MappedPhysicalPageMapper::MemcpyFromPhysical(void* to, phys_addr_t _from,
	size_t length, bool user)
{
	if (_from >= KERNEL_PMAP_SIZE || _from + length > KERNEL_PMAP_SIZE)
		return B_BAD_ADDRESS;

	void* from = (void*)(_from + KERNEL_PMAP_BASE);

	if (user)
		return user_memcpy(to, from, length);
	else
		memcpy(to, from, length);

	return B_OK;
}


status_t
MappedPhysicalPageMapper::MemcpyToPhysical(phys_addr_t _to, const void* from,
	size_t length, bool user)
{
	if (_to >= KERNEL_PMAP_SIZE || _to + length > KERNEL_PMAP_SIZE)
		return B_BAD_ADDRESS;

	void* to = (void*)(_to + KERNEL_PMAP_BASE);

	if (user)
		return user_memcpy(to, from, length);
	else
		memcpy(to, from, length);

	return B_OK;
}


void
MappedPhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to,
	phys_addr_t from)
{
	memcpy((void*)(to + KERNEL_PMAP_BASE), (void*)(from + KERNEL_PMAP_BASE),
		B_PAGE_SIZE);
}


// #pragma mark - Initialization


status_t
mapped_physical_page_ops_init(kernel_args* args,
	X86PhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper)
{
	new(&sPhysicalPageMapper) MappedPhysicalPageMapper;
	new(&sKernelPageMapper) MappedTranslationMapPhysicalPageMapper;

	_pageMapper = &sPhysicalPageMapper;
	_kernelPageMapper = &sKernelPageMapper;
	return B_OK;
}
