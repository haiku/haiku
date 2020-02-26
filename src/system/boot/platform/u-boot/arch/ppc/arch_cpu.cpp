/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
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
#include <arch/ppc/arch_platform.h>
#include <arch_kernel.h>
#include <arch_system_info.h>
#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>

#include <string.h>

// TODO: try to move remaining FDT code to OF calls
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


// FIXME: this is ugly; introduce a cpu type in kernel args
bool gIs440 = false;


static status_t
enumerate_cpus(void)
{
	// iterate through the "/cpus" node to find all CPUs
	int cpus = of_finddevice("/cpus");
	if (cpus == OF_FAILED) {
		printf("enumerate_cpus(): Failed to open \"/cpus\"!\n");
		return B_ERROR;
	}

	char cpuPath[256];
	int cookie = 0;
	int cpuCount = 0;
	while (of_get_next_device(&cookie, cpus, "cpu", cpuPath,
			sizeof(cpuPath)) == B_OK) {
		TRACE(("found CPU: %s\n", cpuPath));

		// For the first CPU get the frequencies of CPU, bus, and time base.
		// We assume they are the same for all CPUs.
		if (cpuCount == 0) {
			int cpu = of_finddevice(cpuPath);
			if (cpu == OF_FAILED) {
				printf("enumerate_cpus: Failed get CPU device node!\n");
				return B_ERROR;
			}

			// TODO: Does encode-int really encode quadlet (32 bit numbers)
			// only?
			int32 clockFrequency;
			if (of_getprop(cpu, "clock-frequency", &clockFrequency, 4)
					== OF_FAILED) {
				printf("enumerate_cpus: Failed to get CPU clock "
					"frequency!\n");
				return B_ERROR;
			}
			int32 busFrequency = 0;
			if (of_getprop(cpu, "bus-frequency", &busFrequency, 4)
					== OF_FAILED) {
				//printf("enumerate_cpus: Failed to get bus clock "
				//	"frequency!\n");
				//return B_ERROR;
			}
			int32 timeBaseFrequency;
			if (of_getprop(cpu, "timebase-frequency", &timeBaseFrequency, 4)
					== OF_FAILED) {
				printf("enumerate_cpus: Failed to get time base "
					"frequency!\n");
				return B_ERROR;
			}

			gKernelArgs.arch_args.cpu_frequency = clockFrequency;
			gKernelArgs.arch_args.bus_frequency = busFrequency;
			gKernelArgs.arch_args.time_base_frequency = timeBaseFrequency;

			TRACE(("  CPU clock frequency: %ld\n", clockFrequency));
			//TRACE(("  bus clock frequency: %ld\n", busFrequency));
			TRACE(("  time base frequency: %ld\n", timeBaseFrequency));
		}

		cpuCount++;
	}

	if (cpuCount == 0) {
		printf("enumerate_cpus(): Found no CPUs!\n");
		return B_ERROR;
	}

	gKernelArgs.num_cpus = cpuCount;

	// FDT doesn't list bus frequency on the cpu node but on the plb node
	if (gKernelArgs.arch_args.bus_frequency == 0) {
		int plb = of_finddevice("/plb");

		int32 busFrequency = 0;
		if (of_getprop(plb, "clock-frequency", &busFrequency, 4)
				== OF_FAILED) {
			printf("enumerate_cpus: Failed to get bus clock "
				"frequency!\n");
			return B_ERROR;
		}
		gKernelArgs.arch_args.bus_frequency = busFrequency;
	}
	TRACE(("  bus clock frequency: %Ld\n",
		gKernelArgs.arch_args.bus_frequency));

#if 0
//XXX:Classic
	// allocate the kernel stacks (the memory stuff is already initialized
	// at this point)
	addr_t stack = (addr_t)arch_mmu_allocate((void*)0x80000000,
		cpuCount * (KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE),
		B_READ_AREA | B_WRITE_AREA, false);
	if (!stack) {
		printf("enumerate_cpus(): Failed to allocate kernel stack(s)!\n");
		return B_NO_MEMORY;
	}

	for (int i = 0; i < cpuCount; i++) {
		gKernelArgs.cpu_kstack[i].start = stack;
		gKernelArgs.cpu_kstack[i].size = KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		stack += KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	}
#endif

	return B_OK;
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

	gIs440 = is_440;

	// some cpu-dependent tweaking

	if (is_440) {
		// the FPU is implemented as an Auxiliary Processing Unit,
		// so we must enable transfers by setting the DAPUIB bit to 0
		// (11th bit)
		asm volatile(
			"mfccr0 %%r3\n"
			"\tlis %%r4,~(1<<(31-11-16))\n"
			"\tand %%r3,%%r3,%%r4\n"
			"\tmtccr0 %%r3"
			: : : "r3", "r4");

		// kernel_args is a packed structure with 64bit fields so...
		// we must enable unaligned transfers by setting the FLSTA bit to 0
		// XXX: actually doesn't work for float ops which gcc emits :-(
		asm volatile(
			"mfccr0 %%r3\n"
			"\tli %%r4,~(1<<(31-23))\n"
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
spin(bigtime_t microseconds)
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
	// This is U-Boot
	gKernelArgs.arch_args.platform = PPC_PLATFORM_U_BOOT;

	// first check some features
	// including some necessary for dprintf()...
	status_t err = check_cpu_features();

	if (err != B_OK) {
		panic("You need a Pentium or higher in order to boot!\n");
		return err;
	}

	// now enumerate correctly all CPUs and get system frequencies
	enumerate_cpus();

	return B_OK;
}

