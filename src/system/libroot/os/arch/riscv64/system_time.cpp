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
	__uint128_t time = static_cast<__uint128_t>(CpuTime()) * cv_factor;
	return time >> 32;
}
