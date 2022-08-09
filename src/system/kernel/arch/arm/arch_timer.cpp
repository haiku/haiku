/*
 * Copyright 2007-2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/stage2.h>
#include <kernel.h>
#include <debug.h>

#include <timer.h>
#include <arch/timer.h>
#include <arch/cpu.h>

#include <drivers/bus/FDT.h>
#include "arch_timer_generic.h"
#include "soc.h"

#include "soc_pxa.h"
#include "soc_omap3.h"

//#define TRACE_ARCH_TIMER
#ifdef TRACE_ARCH_TIMER
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	HardwareTimer *timer = HardwareTimer::Get();
	if (timer != NULL)
		timer->SetTimeout(timeout);
}


void
arch_timer_clear_hardware_timer()
{
	HardwareTimer *timer = HardwareTimer::Get();
	if (timer != NULL)
		timer->Clear();
}

int
arch_init_timer(kernel_args *args)
{
	TRACE("%s\n", __func__);

	if (ARMGenericTimer::IsAvailable()) {
		TRACE("init ARMv7 generic timer\n");
		ARMGenericTimer::Init();
	} else if (strncmp(args->arch_args.timer.kind, TIMER_KIND_OMAP3,
		sizeof(args->arch_args.timer.kind)) == 0) {
		OMAP3Timer::Init(args->arch_args.timer.regs.start,
			args->arch_args.timer.interrupt);
	} else if (strncmp(args->arch_args.timer.kind, TIMER_KIND_PXA,
		sizeof(args->arch_args.timer.kind)) == 0) {
		PXATimer::Init(args->arch_args.timer.regs.start);
	} else {
		panic("No hardware timer found!\n");
	}

	return B_OK;
}


bigtime_t
system_time(void)
{
	HardwareTimer *timer = HardwareTimer::Get();
	if (timer != NULL)
		return timer->Time();

	return 0;
}
