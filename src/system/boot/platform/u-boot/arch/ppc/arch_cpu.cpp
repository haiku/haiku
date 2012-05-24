/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * calculate_cpu_conversion_factor() was written by Travis Geiselbrecht and
 * licensed under the NewOS license.
 */


#include "cpu.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch/ppc/arch_cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

//uint32 gTimeConversionFactor;


static void
calculate_cpu_conversion_factor()
{
	#warning U-Boot:TODO!
}


static status_t
check_cpu_features()
{
	uint32 msr;

	// we do need an FPU
	// on Sam460ex at least U-Boot doesn't enable the FPU for us
	msr = get_msr();
	msr |= MSR_FP_AVAILABLE;
	msr = set_msr(msr);

	if ((msr & MSR_FP_AVAILABLE) == 0) {
		// sadly panic uses vsnprintf which fails without FPU anyway
		panic("no FPU!");
		return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


extern "C" void
arch_spin(bigtime_t microseconds)
{
	for(bigtime_t i=0;i<microseconds;i=i+1)
	{
		/*
		asm volatile ("mov r0,r0");
		asm volatile ("mov r0,r0");
		*/
	}
	#warning U-Boot:PPC:TODO!!
}


extern "C" status_t
boot_arch_cpu_init(void)
{
	status_t err = check_cpu_features();

	if (err != B_OK) {
		panic("You need a Pentium or higher in order to boot!\n");
		return err;
	}

	calculate_cpu_conversion_factor();

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on

	return B_OK;
}

