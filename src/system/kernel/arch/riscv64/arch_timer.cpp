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

static uint64 sTimerConversionFactor;


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
/*
	dprintf("arch_timer_set_hardware_timer(%" B_PRIu64 "), cpu: %" B_PRId32 "\n", timeout,
		smp_get_current_cpu());
*/
	uint64 scaledTimeout = (static_cast<__uint128_t>(timeout) * sTimerConversionFactor) >> 32;

	SetBitsSie(1 << sTimerInt);

	switch (gPlatform) {
		case kPlatformMNative:
			MSyscall(kMSyscallSetTimer, true, gClintRegs->mtime + scaledTimeout);
			break;
		case kPlatformSbi: {
			sbi_set_timer(CpuTime() + scaledTimeout);
			break;
		}
		default:
			;
	}
}


void
arch_timer_clear_hardware_timer()
{
	ClearBitsSie(1 << sTimerInt);

	switch (gPlatform) {
		case kPlatformMNative:
			MSyscall(kMSyscallSetTimer, false);
			break;
		case kPlatformSbi: {
			// Do nothing, it is not possible to clear SBI timer, so we only disable supervisor
			// timer interrupt. SBI timer still can be triggered, but its interrupt will be not
			// delivered to kernel.
			break;
		}
		default:
			;
	}
}


int
arch_init_timer(kernel_args *args)
{
	sTimerConversionFactor = (1LL << 32) * args->arch_args.timerFrequency / 1000000LL;

	return B_OK;
}
