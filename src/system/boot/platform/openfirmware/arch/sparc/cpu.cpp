/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/platform/openfirmware/platform_arch.h>

#include <stdio.h>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <kernel.h>
#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>

//#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
boot_arch_cpu_init(void)
{
	int32 busFrequency = 0;

	intptr_t cpus = of_finddevice("/");
	if (cpus == OF_FAILED) {
		printf("boot_arch_cpu_init(): Failed to open \"/\"!\n");
		return B_ERROR;
	}

	of_getprop(cpus, "clock-frequency", &busFrequency, 4);

	char cpuPath[256];
	intptr_t cookie = 0;
	int cpuCount = 0;
	while (of_get_next_device(&cookie, cpus, "cpu", cpuPath,
			sizeof(cpuPath)) == B_OK) {
		TRACE(("found CPU: %s\n", cpuPath));

		// For the first CPU get the frequencies of CPU, bus, and time base.
		// We assume they are the same for all CPUs.
		if (cpuCount == 0) {
			intptr_t cpu = of_finddevice(cpuPath);
			if (cpu == OF_FAILED) {
				printf("boot_arch_cpu_init: Failed get CPU device node!\n");
				return B_ERROR;
			}

			// TODO: Does encode-int really encode quadlet (32 bit numbers)
			// only?
			int32 clockFrequency;
			if (of_getprop(cpu, "clock-frequency", &clockFrequency, 4)
					== OF_FAILED) {
				printf("boot_arch_cpu_init: Failed to get CPU clock "
					"frequency!\n");
				return B_ERROR;
			}
			if (busFrequency == 0
				&& of_getprop(cpu, "bus-frequency", &busFrequency, 4)
					== OF_FAILED) {
				printf("boot_arch_cpu_init: Failed to get bus clock "
					"frequency!\n");
				return B_ERROR;
			}

			gKernelArgs.arch_args.cpu_frequency = clockFrequency;
			gKernelArgs.arch_args.bus_frequency = busFrequency;
			gKernelArgs.arch_args.time_base_frequency = clockFrequency;

			TRACE(("  CPU clock frequency: %d\n", clockFrequency));
			TRACE(("  bus clock frequency: %d\n", busFrequency));
		}

		cpuCount++;
	}

	if (cpuCount == 0) {
		printf("boot_arch_cpu_init(): Found no CPUs!\n");
		return B_ERROR;
	}

	gKernelArgs.num_cpus = cpuCount;

	// allocate the kernel stacks (the memory stuff is already initialized
	// at this point)
	addr_t stack = (addr_t)arch_mmu_allocate((void*)0x80000000,
		cpuCount * (KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE),
		B_READ_AREA | B_WRITE_AREA, false);
	if (!stack) {
		printf("boot_arch_cpu_init(): Failed to allocate kernel stack(s)!\n");
		return B_NO_MEMORY;
	}

	for (int i = 0; i < cpuCount; i++) {
		gKernelArgs.cpu_kstack[i].start = stack;
		gKernelArgs.cpu_kstack[i].size = KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		stack += KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	}

	return B_OK;
}

