/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "cpu.h"
#include "rom_calls.h"

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
	uint16 flags = SysBase->AttnFlags;
	int cpu = 0;
	int fpu = 0;

	// check fpu flags first, since they are also set for 040

	if ((flags & AFF_68881) != 0)
			fpu = 68881;
	if ((flags & AFF_68882) != 0)
			fpu = 68882;

	//if ((flags & AFF_68010) != 0)
	//	return B_ERROR;
	//if ((flags & AFF_68020) != 0)
	//	return B_ERROR;
	if ((flags & AFF_68030) != 0)
			cpu = 68030;
	if ((flags & AFF_68040) != 0)
			cpu = fpu = 68040;
	//if ((flags & AFF_FPU40) != 0)
	//		;

	//panic("cpu %d fpu %d flags 0x%04x", cpu, fpu, flags);
	cpu = fpu = 68040; //XXX
	if (!cpu || !fpu)
		return B_ERROR;

	gKernelArgs.arch_args.cpu_type = cpu;
	gKernelArgs.arch_args.mmu_type = cpu;
	gKernelArgs.arch_args.fpu_type = fpu;

	//	if ((flags & AFF_68060) != 0) true
	gKernelArgs.arch_args.has_lpstop = false;

	gKernelArgs.arch_args.platform = M68K_PLATFORM_AMIGA;
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

