/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kernel.h>
#include <boot/stage2.h>
#include <arch/vm.h>


int
arch_vm_init(kernel_args *ka)
{
	return 0;
}


int
arch_vm_init2(kernel_args *ka)
{
	return 0;
}


int
arch_vm_init_endvm(kernel_args *ka)
{
	return 0;
}


void
arch_vm_aspace_swap(vm_address_space *aspace)
{
}

