/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <timer.h>
#include <arch/x86/timer.h>
#include <arch/x86/arch_hpet.h>

#include <boot/kernel_args.h>

#include <arch/int.h>
#include <arch/cpu.h>

#include "hpet.h"

#define TRACE_HPET
#ifdef TRACE_HPET
	#define TRACE(x) dprintf x
#else
	#define TRACE(x) ;
#endif

static uint32 sHPETAddr;
static struct hpet_regs *sHPETRegs;

struct timer_info gHPETTimer = {
	"HPET",
	&hpet_get_prio,
	&hpet_set_hardware_timer,
	&hpet_clear_hardware_timer,
	&hpet_init
};


static int
hpet_get_prio(void)
{
	return 3;
}


static int32
hpet_timer_interrupt(void *arg)
{
	return timer_interrupt();
}


static status_t
hpet_set_hardware_timer(bigtime_t relativeTimeout)
{
	cpu_status state;
	uint64 timerValue;

	//dprintf("disabling interrupts\n");

	state = disable_interrupts();

	//dprintf("getting value\n");
	timerValue = relativeTimeout;
	timerValue *= 1000000;
	timerValue /= sHPETRegs->period;
	//dprintf("adding hpet counter value\n");
	timerValue += sHPETRegs->counter;

	//dprintf("setting value %d, cur is %d\n", timerValue, sHPETRegs->counter);
	sHPETRegs->timer2.comparator = timerValue;

	// Clear the interrupt (set to 0)
	//dprintf("clearing interrupts\n");
	sHPETRegs->timer2.config &= (~31 << 9);

	// Non-periodic mode, edge triggered
	//dprintf("edge mode\n");
	sHPETRegs->timer2.config &= ~(0x8 & 0x2);

	// Enable timer
	//dprintf("enable\n");
	sHPETRegs->timer2.config |= 0x4;

	//dprintf("reenable interrupts\n");
	restore_interrupts(state);

	return B_OK;
}


static status_t
hpet_clear_hardware_timer(void)
{
	// Disable timer
	sHPETRegs->timer2.config &= ~0x4;
	return B_OK;
}


static int
hpet_set_enabled(struct hpet_regs *regs, bool enabled)
{
	if (enabled)
		regs->config |= HPET_CONF_MASK_ENABLED;
	else
		regs->config &= ~HPET_CONF_MASK_ENABLED;
	return B_OK;
}


static int
hpet_set_legacy(struct hpet_regs *regs, bool enabled)
{
//	if (!HPET_IS_LEGACY_CAPABLE(regs))
//		return B_NOT_SUPPORTED;

	if (enabled)
		regs->config |= HPET_CONF_MASK_LEGACY;
	else
		regs->config &= ~HPET_CONF_MASK_LEGACY;
	return B_OK;
}


static status_t
hpet_init(struct kernel_args *args)
{
	/* hpet_acpi_probe() through a similar "scan spots" table to that of smp.cpp.
	   Seems to be the most elegant solution right now. */

	if (args->arch_args.hpet == NULL) {
		return B_ERROR;
	}

	if (sHPETRegs == NULL) {
		sHPETRegs = (struct hpet_regs *)args->arch_args.hpet;
		if (map_physical_memory("hpet", (void *)args->arch_args.hpet_phys,
			B_PAGE_SIZE, B_EXACT_ADDRESS, B_KERNEL_READ_AREA |
			B_KERNEL_WRITE_AREA, (void **)&sHPETRegs) < B_OK) {
			// Would it be better to panic here?
			dprintf("hpet_init: Failed to map memory for the HPET registers.");
			return B_ERROR;
		}
	}

	TRACE(("hpet_init: HPET is at %p. Vendor ID: %lx.\n", sHPETRegs, HPET_GET_VENDOR_ID(sHPETRegs)));

	/* There is no hpet legacy support, so error out on init */
	if (!HPET_IS_LEGACY_CAPABLE(sHPETRegs)) {
		dprintf("hpet_init: HPET does support legacy mode. Skipping.\n");
		return B_ERROR;
	}

	hpet_set_legacy(sHPETRegs, true);
	TRACE(("hpet_init: HPET does%s support legacy mode.\n", HPET_IS_LEGACY_CAPABLE(sHPETRegs) ? "" : " not"));
	TRACE(("hpet_init: HPET supports %lu timers, and is %s bits wide.\n", HPET_GET_NUM_TIMERS(sHPETRegs) + 1, HPET_IS_64BIT(sHPETRegs) ? "64" : "32"));

	if (HPET_GET_NUM_TIMERS(sHPETRegs) < 2) {
		dprintf("hpet_init: HPET does not have at least 3 timers. Skipping.\n");
		return B_ERROR;
	}

	install_io_interrupt_handler(0xfb - ARCH_INTERRUPT_BASE, &hpet_timer_interrupt, NULL, B_NO_LOCK_VECTOR);
	install_io_interrupt_handler(0, &hpet_timer_interrupt, NULL, B_NO_LOCK_VECTOR);

	hpet_set_enabled(sHPETRegs, true);
	return B_OK;
}
