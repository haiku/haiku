/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kernel.h>
#include <boot/kernel_args.h>

#include <arch/vm.h>
#include <arch_mmu.h>


int 
arch_vm_init(kernel_args *ka)
{
	return 0;
}


int 
arch_vm_init2(kernel_args *ka)
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
	return 0;
}


int 
arch_vm_init_existing_maps(kernel_args *ka)
{
	void *temp = (void *)ka->fb.mapping.start;

	// create a region for the framebuffer
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "framebuffer", &temp, B_EXACT_ADDRESS,
		ka->fb.mapping.size, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	return B_NO_ERROR;
}


int 
arch_vm_init_endvm(kernel_args *ka)
{
	return B_NO_ERROR;
}


void 
arch_vm_aspace_swap(vm_address_space *aspace)
{
}

