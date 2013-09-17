/*
 * Copyright 2007-2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *              Ithamar R. Adema <ithamar@upgrade-android.com>
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


//#define TRACE_ARCH_TIMER
#ifdef TRACE_ARCH_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define PXA_TIMERS_PHYS_BASE	0x40A00000
#define PXA_TIMERS_SIZE		B_PAGE_SIZE
#define PXA_TIMERS_INTERRUPT	7 /* OST_4_11 */

#define PXA_OSSR		0x05
#define PXA_OIER		0x07
#define PXA_OSCR4		0x10
#define PXA_OSCR5		0x11
#define PXA_OSMR4		0x20
#define PXA_OSMR5		0x21
#define PXA_OMCR4		0x30
#define PXA_OMCR5		0x31

#define PXA_RES_S	(3 << 0)
#define PXA_RES_MS	(1 << 1)
#define PXA_RES_US	(1 << 2)

#define US2S(bt)	((bt) / 1000000ULL)
#define US2MS(bt)	((bt) / 1000ULL)

static area_id sPxaTimersArea = -1;
static uint32 *sPxaTimersBase = NULL;
static bigtime_t sSystemTime = 0;

static int32
pxa_timer_interrupt(void *data)
{
	if (sPxaTimersBase[PXA_OSSR] & (1 << 4)) {
		sPxaTimersBase[PXA_OSSR] |= (1 << 4);
		return timer_interrupt();
	}

	if (sPxaTimersBase[PXA_OSSR]  & (1 << 5)) {
		sPxaTimersBase[PXA_OSSR] |= (1 << 5);
		sSystemTime += UINT_MAX + 1ULL;
	}

	return B_HANDLED_INTERRUPT;
}

void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	uint32 val = timeout & UINT_MAX;
	uint32 res = PXA_RES_US;

	if (timeout & ~UINT_MAX) {
		// Does not fit, so scale resolution down to milliseconds
		if (US2MS(timeout) & ~UINT_MAX) {
			// Still does not fit, scale down to seconds as last ditch attempt
			val = US2S(timeout) & UINT_MAX;
			res = PXA_RES_S;
		} else {
			// Fits in millisecond resolution
			val = US2MS(timeout) & UINT_MAX;
			res = PXA_RES_MS;
		}
	}

	TRACE(("arch_timer_set_hardware_timer(val=%lu, res=%lu)\n", val, res));
	sPxaTimersBase[PXA_OIER] |= (1 << 4);
	sPxaTimersBase[PXA_OMCR4] = res;
	sPxaTimersBase[PXA_OSMR4] = val;
	sPxaTimersBase[PXA_OSCR4] = 0; // start counting from 0 again
}


void
arch_timer_clear_hardware_timer()
{
	TRACE(("arch_timer_clear_hardware_timer\n"));

	sPxaTimersBase[PXA_OMCR4] = 0; // disable our timer
	sPxaTimersBase[PXA_OIER] &= ~(1 << 4);
}

int
arch_init_timer(kernel_args *args)
{
	TRACE(("%s\n", __func__));
	sPxaTimersArea = map_physical_memory("pxa_timers", PXA_TIMERS_PHYS_BASE,
		PXA_TIMERS_SIZE, 0, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&sPxaTimersBase);

	if (sPxaTimersArea < 0)
		return sPxaTimersArea;

	sPxaTimersBase[PXA_OIER] |= (1 << 5); // enable timekeeping timer
	sPxaTimersBase[PXA_OMCR5] = PXA_RES_US | (1 << 7);
	sPxaTimersBase[PXA_OSMR5] = UINT_MAX;
	sPxaTimersBase[PXA_OSCR5] = 0;

	install_io_interrupt_handler(PXA_TIMERS_INTERRUPT, &pxa_timer_interrupt, NULL, 0);

	return B_OK;
}

bigtime_t
system_time(void)
{
	if (sPxaTimersArea < 0)
		return 0;

	return (sPxaTimersBase != NULL) ?
		sSystemTime + sPxaTimersBase[PXA_OSCR5] :
		0ULL;
}
