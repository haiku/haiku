/* Contains the basic code for fault handling. */

/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <faults.h>
#include <faults_priv.h>
#include <debug.h>
#include <arch/int.h>
#include <int.h>
#include <arch/faults.h>
#include <stage2.h>
#include <string.h>
#include <stdio.h>

int faults_init(kernel_args *ka)
{
	dprintf("init_fault_handlers: entry\n");
	return arch_faults_init(ka);
}


int general_protection_fault(int errorcode)
{
	panic("GENERAL PROTECTION FAULT: errcode 0x%x. Killing system.\n", errorcode);

	return INT_NO_RESCHEDULE;
}

static const char *fpu_fault_to_str(enum fpu_faults fpu_fault)
{
	switch(fpu_fault) {
		default:
		case FPU_FAULT_CODE_UNKNOWN:
			return "unknown";
		case FPU_FAULT_CODE_DIVBYZERO:
			return "divbyzero";
		case FPU_FAULT_CODE_INVALID_OP:
			return "invalid op";
		case FPU_FAULT_CODE_OVERFLOW:
			return "overflow";
		case FPU_FAULT_CODE_UNDERFLOW:
			return "underflow";
		case FPU_FAULT_CODE_INEXACT:
			return "inexact";
	}
}

int fpu_fault(int fpu_fault)
{	
	panic("FPU FAULT: errcode 0x%x (%s), Killing system.\n", fpu_fault, fpu_fault_to_str(fpu_fault));

	return INT_NO_RESCHEDULE;
}

int fpu_disable_fault(void)
{
	panic("FPU DISABLE FAULT: Killing system.\n");

	return INT_NO_RESCHEDULE;
}

