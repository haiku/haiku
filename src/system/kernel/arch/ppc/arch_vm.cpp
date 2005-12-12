/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <kernel.h>
#include <boot/kernel_args.h>

#include <arch/vm.h>
#include <arch_mmu.h>


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
	// throw away anything in the kernel_args.pgtable[] that's not yet mapped
	//vm_free_unused_boot_loader_range(KERNEL_BASE, 0x400000 * args->arch_args.num_pgtables);

	return B_OK;
}


void 
arch_vm_aspace_swap(vm_address_space *aspace)
{
}


bool
arch_vm_supports_protection(uint32 protection)
{
	return true;
}


void
arch_vm_init_area(vm_area *area)
{
}


void
arch_vm_unset_memory_type(vm_area *area)
{
}


status_t
arch_vm_set_memory_type(vm_area *area, uint32 type)
{
	if (type == 0)
		return B_OK;

	return B_ERROR;
}
