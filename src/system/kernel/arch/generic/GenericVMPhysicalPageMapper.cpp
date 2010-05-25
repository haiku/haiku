/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "GenericVMPhysicalPageMapper.h"

#include <Errors.h>

#include "generic_vm_physical_page_mapper.h"
#include "generic_vm_physical_page_ops.h"


GenericVMPhysicalPageMapper::GenericVMPhysicalPageMapper()
{
}


GenericVMPhysicalPageMapper::~GenericVMPhysicalPageMapper()
{
}


status_t
GenericVMPhysicalPageMapper::GetPage(phys_addr_t physicalAddress,
	addr_t* _virtualAddress, void** _handle)
{
	return generic_get_physical_page(physicalAddress, _virtualAddress, 0);
}


status_t
GenericVMPhysicalPageMapper::PutPage(addr_t virtualAddress, void* handle)
{
	return generic_put_physical_page(virtualAddress);
}


status_t
GenericVMPhysicalPageMapper::GetPageCurrentCPU(phys_addr_t physicalAddress,
	addr_t* _virtualAddress, void** _handle)
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
GenericVMPhysicalPageMapper::PutPageCurrentCPU(addr_t virtualAddress,
	void* _handle)
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
GenericVMPhysicalPageMapper::GetPageDebug(phys_addr_t physicalAddress,
	addr_t* _virtualAddress, void** _handle)
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
GenericVMPhysicalPageMapper::PutPageDebug(addr_t virtualAddress, void* handle)
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
GenericVMPhysicalPageMapper::MemsetPhysical(phys_addr_t address, int value,
	phys_size_t length)
{
	return generic_vm_memset_physical(address, value, length);
}


status_t
GenericVMPhysicalPageMapper::MemcpyFromPhysical(void* to, phys_addr_t from,
	size_t length, bool user)
{
	return generic_vm_memcpy_from_physical(to, from, length, user);
}


status_t
GenericVMPhysicalPageMapper::MemcpyToPhysical(phys_addr_t to, const void* from,
	size_t length, bool user)
{
	return generic_vm_memcpy_to_physical(to, from, length, user);
}


void
GenericVMPhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to,
	phys_addr_t from)
{
	generic_vm_memcpy_physical_page(to, from);
}
