/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <KernelExport.h>
#include <faults.h>
#include <faults_priv.h>
#include <boot/kernel_args.h>

#include <arch/faults.h>

// XXX this module is largely outdated. Will probably be removed later.

int arch_faults_init(kernel_args *ka)
{
	return 0;
}

int i386_general_protection_fault(int errorcode)
{
	return general_protection_fault(errorcode);
}

int i386_double_fault(int errorcode)
{
	kprintf("double fault! errorcode = 0x%x\n", errorcode);
	dprintf("double fault! errorcode = 0x%x\n", errorcode);
	for(;;);
	return B_HANDLED_INTERRUPT;
}

