/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <kernel/arch/vm.h>
#include <kernel/arch/sh4/cpu.h>
#include <boot/stage2.h>

int arch_vm_init(kernel_args *ka)
{
	return 0;
}

int arch_vm_init2(kernel_args *ka)
{
	return 0;
}

int arch_vm_init_endvm(kernel_args *ka)
{

	dprintf("arch_vm_init_endvm: entry\n");

	return 0;
}

void arch_vm_aspace_swap(vm_address_space *aspace)
{
	sh4_set_user_pgdir(vm_translation_map_get_pgdir(&aspace->translation_map));
}

