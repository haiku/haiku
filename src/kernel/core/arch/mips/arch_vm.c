/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <boot/stage2.h>

int arch_vm_init(kernel_args *ka)
{
	return 0;
}

int arch_vm_init2(kernel_args *ka)
{
	return 0;
}

int map_page_into_kspace(addr paddr, addr kaddr, int lock)
{
	panic("map_page_into_kspace: XXX finish or dont use!\n");
	return 0;
}

