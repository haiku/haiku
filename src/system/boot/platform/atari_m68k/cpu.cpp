/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
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
#warning M68K: TODO: probe ourselves, we shouldnt trust the TOS!

	const tos_cookie *c;
	uint16 cpu_type, fpu_type, fpu_emul;
	uint16 machine_type;
	int fpu = 0;

	c = tos_find_cookie('_CPU');
	if (!c) {
		panic("can't get a cookie (_CPU)!");
		return EINVAL;
	}
	cpu_type = (uint16) c->ivalue;

	dump_tos_cookies();

#warning M68K: check for 020 + mmu
	if (cpu_type < 30/*20*/)
		return EINVAL;
	
#warning M68K: check cpu type passed to kern args
	gKernelArgs.arch_args.cpu_type = 68000 + cpu_type;
	gKernelArgs.arch_args.mmu_type = 68000 + cpu_type;
	gKernelArgs.arch_args.has_lpstop = (cpu_type >= 60)?true:false;

	c = tos_find_cookie('_FPU');
	if (!c) {
		panic("can't get a cookie (_FPU)!");
		return EINVAL;
	}
	fpu_type = (uint16)(c->ivalue >> 16);
	fpu_emul = (uint16)(c->ivalue);

	// http://toshyp.atari.org/003007.htm#Cookie_2C_20_FPU
	if (fpu_type & 0x10)
		fpu = 68060;
	else if (fpu_type & 0x8)
		fpu = 68040;
	else if ((fpu_type & 0x6) == 0x6)
		fpu = 68882;
	else if ((fpu_type & 0x6) == 0x4)
		fpu = 68881;
	else if ((fpu_type & 0x6) == 0x2)
		fpu = 68881; // not certain
	else if (fpu_type & 0x4) {
		panic("bad fpu");
		return EINVAL;
	}
	gKernelArgs.arch_args.fpu_type = fpu;

	gKernelArgs.arch_args.platform = M68K_PLATFORM_ATARI;

	c = tos_find_cookie('_MCH');
	if (!c) {
		panic("can't get a cookie (_MCH)!");
		return EINVAL;
	}
	machine_type = (uint16)(c->ivalue >> 16);
	gKernelArgs.arch_args.machine = machine_type;

	return B_OK;
}

#warning M68K: move and implement system_time()
static bigtime_t gSystemTimeCounter = 0; //HACK
extern "C" bigtime_t
system_time(void)
{
	// _hz_200
	//return (*TOSVAR_hz_200) * 1000000LL / 200;
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
		panic("You need a 68020 or higher in order to boot!\n");

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
		// ...or not!
}


extern "C" void
platform_load_ucode(BootVolume& volume)
{
}

