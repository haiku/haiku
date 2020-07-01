/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <boot/arch/x86/arch_cpu.h>
#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <arch/x86/arch_cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define CPUID_EFLAGS	(1UL << 21)
#define RDTSC_FEATURE	(1UL << 4)


static status_t
check_cpu_features()
{
	// check the eflags register to see if the cpuid instruction exists
	if ((get_eflags() & CPUID_EFLAGS) == 0) {
		// it's not set yet, but maybe we can set it manually
		set_eflags(get_eflags() | CPUID_EFLAGS);
		if ((get_eflags() & CPUID_EFLAGS) == 0)
			return B_ERROR;
	}

	cpuid_info info;
	if (get_current_cpuid(&info, 1, 0) != B_OK)
		return B_ERROR;

	if ((info.eax_1.features & RDTSC_FEATURE) == 0) {
		// we currently require RDTSC
		return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


void
cpu_init()
{
	if (check_cpu_features() != B_OK)
		panic("You need a Pentium or higher in order to boot!\n");

	calculate_cpu_conversion_factor(0);

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
}


extern "C" void
platform_load_ucode(BootVolume& volume)
{
	arch_ucode_load(volume);
}
