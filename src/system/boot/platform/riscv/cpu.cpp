/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "cpu.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_cpu_defs.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
# define TRACE(x...) dprintf(x)
#else
# define TRACE(x...) ;
#endif


//	#pragma mark -


extern "C" void
cpu_init()
{
	gKernelArgs.num_cpus = 1;

	SstatusReg status(Sstatus());
	status.fs = extStatusInitial; // enable FPU
	status.xs = extStatusOff;
	SetSstatus(status.val);
}


extern "C" void
platform_load_ucode(BootVolume& volume)
{
	// we have no ucode
}
