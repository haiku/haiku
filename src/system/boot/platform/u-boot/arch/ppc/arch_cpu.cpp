/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * calculate_cpu_conversion_factor() was written by Travis Geiselbrecht and
 * licensed under the NewOS license.
 */


#include "cpu.h"
#include "board_config.h"

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

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};


extern void *gFDT;

#define TRACE_CPU
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
	uint32 pvr;
	bool is_440 = false;
	bool is_460 = false;
	bool pvr_unknown = false;
	bool fdt_unknown = true;
	const char *fdt_model = NULL;

	// attempt to detect processor version, either from PVR or FDT
	// TODO:someday we'll support other things than 440/460...

	// read the Processor Version Register
	pvr = get_pvr();
	uint16 version = (uint16)(pvr >> 16);
	uint16 revision = (uint16)(pvr & 0xffff);

	switch (version) {
		case AMCC440EP:
			is_440 = true;
			break;
		case AMCC460EX:
			is_460 = true;
			break;
		default:
			pvr_unknown = true;
	}

	// if we have an FDT...
	// XXX: use it only as fallback?
	if (gFDT != NULL/* && pvr_unknown*/) {
		// TODO: for MP support we must check /chosen/cpu first
		int node = fdt_path_offset(gFDT, "/cpus/cpu@0");
		int len;

		fdt_model = (const char *)fdt_getprop(gFDT, node, "model", &len);
		// TODO: partial match ? "PowerPC,440" && isalpha(next char) ?
		if (fdt_model) {
			if (!strcmp(fdt_model, "PowerPC,440EP")) {
				is_440 = true;
				fdt_unknown = false;
			} else if (!strcmp(fdt_model, "PowerPC,460EX")) {
				is_460 = true;
				fdt_unknown = false;
			}
		}
	}

	if (is_460)
		is_440 = true;

	// some cpu-dependent tweaking

	if (is_440) {
		// the FPU is implemented as an Auxiliary Processing Unit,
		// so we must enable transfers by setting the DAPUIB bit to 0
		asm volatile(
			"mfccr0 %%r3\n"
			"\tlis %%r4,~(1<<(20-16))\n"
			"\tand %%r3,%%r3,%%r4\n"
			"\tmtccr0 %%r3"
			: : : "r3", "r4");
	}

	// we do need an FPU for vsnprintf to work
	// on Sam460ex at least U-Boot doesn't enable the FPU for us
	msr = get_msr();
	msr |= MSR_FP_AVAILABLE;
	msr = set_msr(msr);

	if ((msr & MSR_FP_AVAILABLE) == 0) {
		// sadly panic uses vsnprintf which fails without FPU anyway
		panic("no FPU!");
		return B_ERROR;
	}

	TRACE(("CPU detection:\n"));
	TRACE(("PVR revision %sknown: 0x%8lx\n", pvr_unknown ? "un" : "", pvr));
	TRACE(("FDT model %sknown: %s\n", fdt_unknown ? "un" : "", fdt_model));
	TRACE(("flags: %s440 %s460\n", is_440 ? "" : "!", is_460 ? "" : "!"));

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
		// TODO:iterate on /cpus/*

	return B_OK;
}

