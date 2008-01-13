/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * calculate_cpu_conversion_factor() was written by Travis Geiselbrecht and
 * licensed under the NewOS license.
 */


#include "cpu.h"
#include "toscalls.h"

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

#warning M68K: add set_vbr()

static status_t
check_cpu_features()
{
#warning M68K: TODO: probe ourselves, we shouldn't trust the TOS!

	const tos_cookie *c;
	c = tos_find_cookie('_CPU');
	if (!c) {
		panic("can't get a cookie (_CPU)! Mum, I'm hungry!");
		return EINVAL;
	}

#warning M68K: check for 020 + mmu
	if (c->ivalue < 30/*20*/)
		return EINVAL;
	
	gKernelArgs.arch_args.has_lpstop = (c->ivalue >= 60)?true:false;
#warning M68K: add cpu type to kern args

	c = tos_find_cookie('_FPU');
	if (!c) {
		panic("can't get a cookie (_FPU)! Mum, I'm hungry!");
		return EINVAL;
	}

#warning M68K: check for fpu in detail
	if (c->ivalue < 2 || c->ivalue > 7) {
		panic("bad fpu");
		return EINVAL;
	}

	return B_OK;
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
		panic("You need a 68020 or higher in order to boot!\n");

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
		// ...or not!
}

