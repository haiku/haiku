/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <KernelExport.h>
#include <faults.h>
#include <faults_priv.h>
#include <boot/kernel_args.h>

#include <arch/cpu.h>
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

int i386_double_fault(struct iframe *frame)
{
	struct tss *tss = x86_get_main_tss();
	
	frame->cs = tss->cs;
	frame->eip = tss->eip;
	frame->esp = tss->esp;
	frame->flags = tss->eflags;
	
	panic("double fault! errorcode = 0x%x\n", frame->error_code);

	return B_HANDLED_INTERRUPT;
}

