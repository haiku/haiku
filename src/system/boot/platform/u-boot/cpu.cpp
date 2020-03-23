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
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern "C" void
cpu_init()
{
	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on

	if (boot_arch_cpu_init() != B_OK)
		panic("You need a 6502 or higher in order to boot!\n");
}


extern "C" void
platform_load_ucode(BootVolume& volume)
{
}
