/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <KernelExport.h>

#include <kernel.h>
#include <boot/kernel_args.h>

#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <arch/vm.h>
#include <arch_mmu.h>


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
arch_vm_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_init2(kernel_args *args)
{
//	int bats[8];
//	int i;

#if 0
	// print out any bat mappings
	getibats(bats);
	dprintf("ibats:\n");
	for(i = 0; i < 4; i++)
		dprintf("0x%x 0x%x\n", bats[i*2], bats[i*2+1]);
	getdbats(bats);
	dprintf("dbats:\n");
	for(i = 0; i < 4; i++)
		dprintf("0x%x 0x%x\n", bats[i*2], bats[i*2+1]);
#endif

#if 1
	// turn off the first 2 BAT mappings (3 & 4 are used by the lower level code)
	block_address_translation bat;
	bat.Clear();

	set_ibat0(&bat);
	set_ibat1(&bat);
	set_dbat0(&bat);
	set_dbat1(&bat);
/*	getibats(bats);
	memset(bats, 0, 2 * 2);
	setibats(bats);
	getdbats(bats);
	memset(bats, 0, 2 * 2);
	setdbats(bats);
*/
#endif
#if 0
	// just clear the first BAT mapping (0 - 256MB)
	dprintf("msr 0x%x\n", getmsr());
	{
		unsigned int reg;
		asm("mr	%0,1" : "=r"(reg));
		dprintf("sp 0x%x\n", reg);
	}
	dprintf("ka %p\n", ka);

	getibats(bats);
	dprintf("ibats:\n");
	for(i = 0; i < 4; i++)
		dprintf("0x%x 0x%x\n", bats[i*2], bats[i*2+1]);
	bats[0] = bats[1] = 0;
	setibats(bats);
	getdbats(bats);
	dprintf("dbats:\n");
	for(i = 0; i < 4; i++)
		dprintf("0x%x 0x%x\n", bats[i*2], bats[i*2+1]);
	bats[0] = bats[1] = 0;
	setdbats(bats);
#endif
	return B_OK;
}


status_t
arch_vm_init_post_area(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_init_end(kernel_args *args)
{
	TRACE(("arch_vm_init_end(): %lu virtual ranges to keep:\n",
		args->arch_args.num_virtual_ranges_to_keep));

	for (int i = 0; i < (int)args->arch_args.num_virtual_ranges_to_keep; i++) {
		addr_range &range = args->arch_args.virtual_ranges_to_keep[i];

		TRACE(("  start: %p, size: 0x%lx\n", (void*)range.start, range.size));

		// skip ranges outside the kernel address space
		if (!IS_KERNEL_ADDRESS(range.start)) {
			TRACE(("    no kernel address, skipping...\n"));
			continue;
		}

		phys_addr_t physicalAddress;
		void *address = (void*)range.start;
		if (vm_get_page_mapping(VMAddressSpace::KernelID(), range.start,
				&physicalAddress) != B_OK)
			panic("arch_vm_init_end(): No page mapping for %p\n", address);
		area_id area = vm_map_physical_memory(VMAddressSpace::KernelID(),
			"boot loader reserved area", &address,
			B_EXACT_ADDRESS, range.size,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			physicalAddress, true);
		if (area < 0) {
			panic("arch_vm_init_end(): Failed to create area for boot loader "
				"reserved area: %p - %p\n", (void*)range.start,
				(void*)(range.start + range.size));
		}
	}

	// Throw away any address space mappings we've inherited from the boot
	// loader and have not yet turned into an area.
	vm_free_unused_boot_loader_range(0, 0xffffffff - B_PAGE_SIZE + 1);

	return B_OK;
}


status_t
arch_vm_init_post_modules(kernel_args *args)
{
	return B_OK;
}


void
arch_vm_aspace_swap(struct VMAddressSpace *from, struct VMAddressSpace *to)
{
}


bool
arch_vm_supports_protection(uint32 protection)
{
	return true;
}


void
arch_vm_unset_memory_type(VMArea *area)
{
}


status_t
arch_vm_set_memory_type(VMArea *area, phys_addr_t physicalBase, uint32 type)
{
	if (type == 0)
		return B_OK;

	return B_OK;
}
