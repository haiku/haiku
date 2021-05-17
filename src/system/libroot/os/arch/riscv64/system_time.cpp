/*
 * Copyright 2012-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */

#include <OS.h>

#include <arch_cpu.h>
#include <arch_cpu_defs.h>
#include <libroot_private.h>
#include <real_time_data.h>

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#endif


static uint64_t cv_factor = 0;


extern "C" void
__riscv64_setup_system_time(uint64 cv)
{
	cv_factor = cv;
}


[[gnu::optimize("omit-frame-pointer")]] bigtime_t
system_time()
{
	// TODO: Timer unit conversion needs fixed here
	// QEMU and SiFive boards use diferent timer frequencies
	return CpuTime();
/*
	uint64 time = CpuTime();
	uint64 lo = (uint32)time;
	uint64 hi = time >> 32;
	return ((lo * cv_factor) >> 32) + hi * cv_factor;
*/
/*
	__uint128_t time = static_cast<__uint128_t>(CpuTime()) * cv_factor;
	return time >> 32;
*/
}
