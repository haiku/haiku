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
#define PXA_TIMERS_SIZE		0x000000C0
#define PXA_TIMERS_INTERRUPT	7 /* OST_4_11 */

#define PXA_OSSR		0x05
#define PXA_OIER		0x07
#define PXA_OSCR4		0x10
#define PXA_OSMR4		0x20
#define PXA_OMCR4		0x30


static area_id sPxaTimersArea;
static uint32 *sPxaTimersBase;


static int32
pxa_timer_interrupt(void *data)
{
	int32 ret = timer_interrupt();
        sPxaTimersBase[PXA_OSSR] |= (1 << 4);

        return ret;
}


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	TRACE(("arch_timer_set_hardware_timer(%lld)\n", timeout));

	if (sPxaTimersBase) {
		sPxaTimersBase[PXA_OIER] |= (1 << 4);
		sPxaTimersBase[PXA_OMCR4] = 4; // set to exactly single milisecond resolution
		sPxaTimersBase[PXA_OSMR4] = timeout;
		sPxaTimersBase[PXA_OSCR4] = 0; // start counting from 0 again
	}
}


void
arch_timer_clear_hardware_timer()
{
	TRACE(("arch_timer_clear_hardware_timer\n"));

	if (sPxaTimersBase) {
		sPxaTimersBase[PXA_OMCR4] = 0; // disable our timer
		sPxaTimersBase[PXA_OIER] &= ~(4 << 1);
	}
}


int
arch_init_timer(kernel_args *args)
{
	sPxaTimersArea = map_physical_memory("pxa_timers", PXA_TIMERS_PHYS_BASE,
		PXA_TIMERS_SIZE, 0, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&sPxaTimersBase);

	if (sPxaTimersArea < 0)
		return sPxaTimersArea;

	sPxaTimersBase[PXA_OMCR4] = 0; // disable our timer

	install_io_interrupt_handler(PXA_TIMERS_INTERRUPT, &pxa_timer_interrupt, NULL, 0);

	return B_OK;
}
