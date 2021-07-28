/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "cpu.h"
#include "nextrom.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_platform.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#warning M68K: add set_vbr()


static status_t
check_cpu_features()
{
	int cpu = 0;
	int fpu = 0;

	//TODO: The ROM only tells which machine. NetBSD probes using CACR instead.
	cpu = fpu = 68040;
	if (!cpu || !fpu)
		return B_ERROR;

	gKernelArgs.arch_args.cpu_type = cpu;
	gKernelArgs.arch_args.mmu_type = cpu;
	gKernelArgs.arch_args.fpu_type = fpu;

	//	if ((flags & AFF_68060) != 0) true
	gKernelArgs.arch_args.has_lpstop = false;

	gKernelArgs.arch_args.platform = M68K_PLATFORM_NEXT;
	gKernelArgs.arch_args.machine = 0; //XXX

	return B_OK;
}

#warning M68K: move and implement system_time()
static bigtime_t gSystemTimeCounter = 0; //HACK
extern "C" bigtime_t
system_time(void)
{
	return gSystemTimeCounter++;
}


//	#pragma mark -


extern "C" void
spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();
	while ((system_time() - time) < microseconds)
		asm volatile ("nop;");
}


extern "C" void
cpu_init()
{
	if (check_cpu_features() != B_OK)
		panic("You need a 68030 or higher in order to boot!\n");

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
		// ...or not!
}


void
platform_load_ucode(BootVolume& volume)
{
}

