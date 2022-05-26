/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <OS.h>
#include <boot/platform.h>
/*
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>
#include <string.h>
*/

extern "C" status_t
boot_arch_cpu_init(void)
{
	return B_OK;
}


extern "C" void
arch_ucode_load(BootVolume& volume)
{
	// Yes, we have no bananas!
}


extern "C" bigtime_t
system_time()
{

	#warning Implement system_time in ARM64 bootloader!
	//https://en.wikipedia.org/wiki/Time_Stamp_Counter
	// read system time register:  cntpct_el0 / cntvct_el0
	// frequency of the system timer can be discovered by reading cntfrq_el0
	// https://wiki.osdev.org/ARMv7_Generic_Timers#CNTPCT

	uint64_t ticks;
	__asm__ volatile("mrs %0, cntpct_el0" : "=r"(ticks));
	//conversion factor stuff
	return ticks;
	//return u64_mul_u64_fp32_64(ticks, ns_per_cntpct);
}


extern "C" void
spin(bigtime_t microseconds)
{
	auto time = system_time();

	while ((system_time() - time) < microseconds)
		asm volatile ("yield;");
}
