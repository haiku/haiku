/*
 * Copyright 2019-2021, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include <arch_cpu_defs.h>
#include <arch_int.h>
#include <arch/timer.h>
#include <boot/kernel_args.h>
#include <debug.h>
#include <kernel.h>
#include <platform/sbi/sbi_syscalls.h>
#include <timer.h>
#include <Clint.h>

#include <smp.h>


extern uint32 gPlatform;


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	// dprintf("arch_timer_set_hardware_timer(%" B_PRIu64 "), cpu: %" B_PRId32 "\n", timeout, smp_get_current_cpu());

	// TODO: Read timer frequency from FDT
	switch (gPlatform) {
		case kPlatformMNative:
			MSyscall(kMSyscallSetTimer, true,
				gClintRegs->mtime + timeout * 10);
			break;
		case kPlatformSbi: {
			sbi_set_timer(CpuTime() + timeout);
			break;
		}
		default:
			;
	}

	// SetSie(Sie() | (1 << sTimerInt));
}


void
arch_timer_clear_hardware_timer()
{
	// SetSie(Sie() & ~(1 << sTimerInt));

	switch (gPlatform) {
		case kPlatformMNative:
			MSyscall(kMSyscallSetTimer, false);
			break;
		case kPlatformSbi: {
			sbi_set_timer(CpuTime() + (uint64)10000000 * 3600);
			break;
		}
		default:
			;
	}
}


int
arch_init_timer(kernel_args *args)
{
	return B_OK;
}
