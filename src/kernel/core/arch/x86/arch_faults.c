/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <faults.h>
#include <faults_priv.h>
#include <vm.h>
#include <debug.h>
#include <console.h>
#include <int.h>

#include <arch/cpu.h>
#include <arch/int.h>
#include <arch/faults.h>

#include <arch/x86/interrupts.h>
#include <arch/x86/faults.h>

#include <stage2.h>

#include <string.h>

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
	return INT_NO_RESCHEDULE;
}

