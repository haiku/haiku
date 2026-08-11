/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_TIMER
#define _KERNEL_ARCH_X86_TIMER


extern uint32 gSystemTimeConversionFactor;
extern int64 gCPUClockSpeed;


void x86_init_tsc(kernel_args* args);


#endif

