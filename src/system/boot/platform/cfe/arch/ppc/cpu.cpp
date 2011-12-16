/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/platform/cfe/platform_arch.h>

#include <stdio.h>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <kernel.h>
//#include <platform/cfe/devices.h>
#include <boot/platform/cfe/cfe.h>

#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
boot_arch_cpu_init(void)
{
#warning PPC:TODO
	return B_ERROR;
}

