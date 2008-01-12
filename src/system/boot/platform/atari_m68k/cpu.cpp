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
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

bool gCpuHasLPSTOP = false;

static status_t
check_cpu_features()
{
#warning M68K: check for LPSTOP
	if (false)
		gCpuHasLPSTOP = true;

#warning M68K: check for >= 020

	return B_OK;
}


//	#pragma mark -


extern "C" void
spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();
	if (gCpuHasLPSTOP) {
		while ((system_time() - time) < microseconds)
			asm volatile ("lpstop;");
	} else {
		while ((system_time() - time) < microseconds)
			asm volatile ("nop;");
	}
}


extern "C" void
cpu_init()
{
	if (check_cpu_features() != B_OK)
		panic("You need a 68020 or higher in order to boot!\n");

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
		// ...or not!
}

