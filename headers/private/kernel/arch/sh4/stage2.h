/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _SH4_STAGE2_H
#define _SH4_STAGE2_H

#include <boot/stage2_struct.h>

#include <arch/sh4/vcpu_struct.h>

// kernel args
typedef struct {
	// architecture specific
	vcpu_struct *vcpu; 
} arch_kernel_args;

#endif

