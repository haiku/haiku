/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*	(bonefish) Some explanatory words on how address translation is implemented
	for the 32 bit PPC architecture.

	I use the address type nomenclature as used in the PPC architecture
	specs, i.e.
	- effective address: An address as used by program instructions, i.e.
	  that's what elsewhere (e.g. in the VM implementation) is called
	  virtual address.
	- virtual address: An intermediate address computed from the effective
	  address via the segment registers.
	- physical address: An address referring to physical storage.

	The hardware translates an effective address to a physical address using
	either of two mechanisms: 1) Block Address Translation (BAT) or
	2) segment + page translation. The first mechanism does this directly
	using two sets (for data/instructions) of special purpose registers.
	The latter mechanism is of more relevance here, though:

	effective address (32 bit):	     [ 0 ESID  3 | 4  PIX 19 | 20 Byte 31 ]
								           |           |            |
							     (segment registers)   |            |
									       |           |            |
	virtual address (52 bit):   [ 0      VSID 23 | 24 PIX 39 | 40 Byte 51 ]
	                            [ 0             VPN       39 | 40 Byte 51 ]
								                 |                  |
										   (page table)             |
											     |                  |
	physical address (32 bit):       [ 0        PPN       19 | 20 Byte 31 ]


	ESID: Effective Segment ID
	VSID: Virtual Segment ID
	PIX:  Page Index
	VPN:  Virtual Page Number
	PPN:  Physical Page Number


	Unlike on x86 we can't just switch the context to another team by just
	setting a register to another page directory, since we only have one
	page table containing both kernel and user address mappings. Instead we
	map the effective address space of kernel and *all* teams
	non-intersectingly into the virtual address space (which fortunately is
	20 bits wider), and use the segment registers to select the section of
	the virtual address space for the current team. Half of the 16 segment
	registers (8 - 15) map the kernel addresses, so they remain unchanged.

	The range of the virtual address space a team's effective address space
	is mapped to is defined by its PPCVMTranslationMap::fVSIDBase,
	which is the first of the 8 successive VSID values used for the team.

	Which fVSIDBase values are already taken is defined by the set bits in
	the bitmap sVSIDBaseBitmap.


	TODO:
	* If we want to continue to use the OF services, we would need to add
	  its address mappings to the kernel space. Unfortunately some stuff
	  (especially RAM) is mapped in an address range without the kernel
	  address space. We probably need to map those into each team's address
	  space as kernel read/write areas.
	* The current locking scheme is insufficient. The page table is a resource
	  shared by all teams. We need to synchronize access to it. Probably via a
	  spinlock.
 */

#include <arch/vm_translation_map.h>

#include <stdlib.h>

#include <KernelExport.h>

#include <arch/cpu.h>
//#include <arch_mmu.h>
#include <boot/kernel_args.h>
#include <interrupts.h>
#include <kernel.h>
#include <slab/Slab.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>

#include <util/AutoLock.h>

#include "generic_vm_physical_page_mapper.h"
//#include "generic_vm_physical_page_ops.h"
//#include "GenericVMPhysicalPageMapper.h"

#include "paging/PPCVMTranslationMap.h"
#include "paging/classic/PPCPagingMethodClassic.h"
//#include "paging/460/PPCPagingMethod460.h"


#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


static union {
	uint64	align;
	//char	amcc460[sizeof(PPCPagingMethod460)];
	char	classic[sizeof(PPCPagingMethodClassic)];
} sPagingMethodBuffer;


#if 0
struct PPCVMTranslationMap : VMTranslationMap {
								PPCVMTranslationMap();
	virtual						~PPCVMTranslationMap();

			status_t			Init(bool kernel);

	inline	int					VSIDBase() const	{ return fVSIDBase; }

			page_table_entry*	LookupPageTableEntry(addr_t virtualAddress);
			bool				RemovePageTableEntry(addr_t virtualAddress);

	virtual	bool	 			Lock();
	virtual	void				Unlock();

	virtual	addr_t				MappedSize() const;
	virtual	size_t				MaxPagesNeededToMap(addr_t start,
									addr_t end) const;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation);
	virtual	status_t			Unmap(addr_t start, addr_t end);

	virtual	status_t			UnmapPage(VMArea* area, addr_t address,
									bool updatePageQueue);

	virtual	status_t			Query(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);
	virtual	status_t			QueryInterrupt(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);

	virtual	status_t			Protect(addr_t base, addr_t top,
									uint32 attributes, uint32 memoryType);
	virtual	status_t			ClearFlags(addr_t virtualAddress,
									uint32 flags);

	virtual	bool				ClearAccessedAndModified(
									VMArea* area, addr_t address,
									bool unmapIfUnaccessed,
									bool& _modified);

	virtual	void				Flush();

protected:
			int					fVSIDBase;
};
#endif


void
ppc_translation_map_change_asid(VMTranslationMap *map)
{
	static_cast<PPCVMTranslationMap*>(map)->ChangeASID();
}


// #pragma mark -


#if 0//XXX:Not needed anymore ?
addr_t
PPCVMTranslationMap::MappedSize() const
{
	return fMapCount;
}


static status_t
get_physical_page_tmap(phys_addr_t physicalAddress, addr_t *_virtualAddress,
	void **handle)
{
	return generic_get_physical_page(physicalAddress, _virtualAddress, 0);
}


static status_t
put_physical_page_tmap(addr_t virtualAddress, void *handle)
{
	return generic_put_physical_page(virtualAddress);
}
#endif


//  #pragma mark -
//  VM API


status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	return gPPCPagingMethod->CreateTranslationMap(kernel, _map);
}


status_t
arch_vm_translation_map_init(kernel_args *args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("vm_translation_map_init: entry\n");

#ifdef TRACE_VM_TMAP
	TRACE("physical memory ranges:\n");
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		phys_addr_t start = args->physical_memory_range[i].start;
		phys_addr_t end = start + args->physical_memory_range[i].size;
		TRACE("  %#10" B_PRIxPHYSADDR " - %#10" B_PRIxPHYSADDR "\n", start,
			end);
	}

	TRACE("allocated physical ranges:\n");
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		phys_addr_t start = args->physical_allocated_range[i].start;
		phys_addr_t end = start + args->physical_allocated_range[i].size;
		TRACE("  %#10" B_PRIxPHYSADDR " - %#10" B_PRIxPHYSADDR "\n", start,
			end);
	}

	TRACE("allocated virtual ranges:\n");
	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		addr_t start = args->virtual_allocated_range[i].start;
		addr_t end = start + args->virtual_allocated_range[i].size;
		TRACE("  %#10" B_PRIxADDR " - %#10" B_PRIxADDR "\n", start, end);
	}
#endif

	if (false /* TODO:Check for AMCC460! */) {
		dprintf("using AMCC 460 paging\n");
		panic("XXX");
		//XXX:gPPCPagingMethod = new(&sPagingMethodBuffer) PPCPagingMethod460;
	} else {
		dprintf("using Classic paging\n");
		gPPCPagingMethod = new(&sPagingMethodBuffer) PPCPagingMethodClassic;
	}

	return gPPCPagingMethod->Init(args, _physicalPageMapper);
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	TRACE("vm_translation_map_init_post_area: entry\n");

	return gPPCPagingMethod->InitPostArea(args);
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	// init physical page mapper
	return generic_vm_physical_page_mapper_init_post_sem(args);
}


/**	Directly maps a page without having knowledge of any kernel structures.
 *	Used only during VM setup.
 *	It currently ignores the "attributes" parameter and sets all pages
 *	read/write.
 */

status_t
arch_vm_translation_map_early_map(kernel_args *args, addr_t va, phys_addr_t pa,
	uint8 attributes)
{
	TRACE("early_tmap: entry pa %#" B_PRIxPHYSADDR " va %#" B_PRIxADDR "\n", pa,
		va);

	return gPPCPagingMethod->MapEarly(args, va, pa, attributes);
}


// XXX currently assumes this translation map is active

status_t
arch_vm_translation_map_early_query(addr_t va, phys_addr_t *out_physical)
{
	//PANIC_UNIMPLEMENTED();
	panic("vm_translation_map_early_query(): not yet implemented\n");
	return B_OK;
}


// #pragma mark -


status_t
ppc_map_address_range(addr_t virtualAddress, phys_addr_t physicalAddress,
	size_t size)
{
	addr_t virtualEnd = ROUNDUP(virtualAddress + size, B_PAGE_SIZE);
	virtualAddress = ROUNDDOWN(virtualAddress, B_PAGE_SIZE);
	physicalAddress = ROUNDDOWN(physicalAddress, B_PAGE_SIZE);

	VMAddressSpace *addressSpace = VMAddressSpace::Kernel();
	PPCVMTranslationMap* map = static_cast<PPCVMTranslationMap*>(
		addressSpace->TranslationMap());

	vm_page_reservation reservation;
	vm_page_reserve_pages(&reservation, 0, VM_PRIORITY_USER);
		// We don't need any pages for mapping.

	// map the pages
	for (; virtualAddress < virtualEnd;
		 virtualAddress += B_PAGE_SIZE, physicalAddress += B_PAGE_SIZE) {
		status_t error = map->Map(virtualAddress, physicalAddress,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0, &reservation);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


void
ppc_unmap_address_range(addr_t virtualAddress, size_t size)
{
	addr_t virtualEnd = ROUNDUP(virtualAddress + size, B_PAGE_SIZE);
	virtualAddress = ROUNDDOWN(virtualAddress, B_PAGE_SIZE);

	VMAddressSpace *addressSpace = VMAddressSpace::Kernel();

	PPCVMTranslationMap* map = static_cast<PPCVMTranslationMap*>(
		addressSpace->TranslationMap());
	map->Unmap(virtualAddress, virtualEnd);
}


status_t
ppc_remap_address_range(addr_t *_virtualAddress, size_t size, bool unmap)
{
	VMAddressSpace *addressSpace = VMAddressSpace::Kernel();

	PPCVMTranslationMap* map = static_cast<PPCVMTranslationMap*>(
		addressSpace->TranslationMap());

	return map->RemapAddressRange(_virtualAddress, size, unmap);
}


bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	if (!gPPCPagingMethod)
		return true;

	return gPPCPagingMethod->IsKernelPageAccessible(virtualAddress, protection);
}
