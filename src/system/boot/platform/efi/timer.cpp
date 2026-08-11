/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
*/

#include "timer.h"

#include <KernelExport.h>

#include "arch_timer.h"
#include "efi_platform.h"


static bigtime_t sSystemTime = 0;
static efi_event sSystemTimeTickEvent;


EFIAPI static void
system_time_tick(efi_event, void*)
{
	sSystemTime += 50 * 1000; /* 50ms, in microseconds */
}


bigtime_t
system_time()
{
	// EFI does not have any mechanism to fetch a current time with a guaranteed
	// subsecond precision, so use a timer event. To avoid unnecessary overhead,
	// we limit it to 50ms granularity, and only start it when first needed.
	if (sSystemTimeTickEvent == NULL) {
		efi_status status = kBootServices->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL,
			TPL_CALLBACK, system_time_tick, NULL, &sSystemTimeTickEvent);
		if (status == EFI_SUCCESS) {
			status = kBootServices->SetTimer(sSystemTimeTickEvent, TimerPeriodic,
				500000 /* 50ms, in 100ns units */);
		}
		if (status != EFI_SUCCESS)
			panic("Can't start system_time event!");
	}

	return sSystemTime;
}


void
spin(bigtime_t microseconds)
{
	kBootServices->Stall(microseconds);
}


// #pragma mark -


void
timer_init()
{
	arch_timer_init();
}
