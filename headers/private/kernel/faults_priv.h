/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_FAULTS_PRIV_H
#define _KERNEL_FAULTS_PRIV_H

int general_protection_fault(int errorcode);

enum fpu_faults {
	FPU_FAULT_CODE_UNKNOWN = 0,
	FPU_FAULT_CODE_DIVBYZERO,
	FPU_FAULT_CODE_INVALID_OP,
	FPU_FAULT_CODE_OVERFLOW,
	FPU_FAULT_CODE_UNDERFLOW,
	FPU_FAULT_CODE_INEXACT
};
int fpu_fault(int fpu_fault);

int fpu_disable_fault(void);

#endif

