/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <timer.h>
#include <arch/x86/timer.h>

#include <int.h>
#include <arch/x86/arch_apic.h>

#include <arch/cpu.h>
#include <arch/smp.h>

#include "apic.h"

static void *sApicPtr = NULL;
static uint32 sApicTicsPerSec = 0;

extern bool gUsingIOAPIC;

struct timer_info gAPICTimer = {
	"APIC",
	&apic_get_prio,
	&apic_set_hardware_timer,
	&apic_clear_hardware_timer,
	&apic_init
};


static int
apic_get_prio(void)
{
	return 2;
}


static uint32
_apic_read(uint32 offset)
{
	return *(volatile uint32 *)((char *)sApicPtr + offset);
}


static void
_apic_write(uint32 offset, uint32 data)
{
	*(volatile uint32 *)((char *)sApicPtr + offset) = data;
}


static int32
apic_timer_interrupt(void *data)
{
	// if we are not using the IO APIC we need to acknowledge the
	// interrupt ourselfs
	if (!gUsingIOAPIC)
		_apic_write(APIC_EOI, 0);

	return timer_interrupt();
}


#define MIN_TIMEOUT 1

static status_t
apic_set_hardware_timer(bigtime_t relativeTimeout)
{
	cpu_status state;
	uint32 config;
	uint32 ticks;

	if (sApicPtr == NULL)
		return B_ERROR;

	if (relativeTimeout < MIN_TIMEOUT)
		relativeTimeout = MIN_TIMEOUT;

	// calculation should be ok, since it's going to be 64-bit
	ticks = ((relativeTimeout * sApicTicsPerSec) / 1000000);

	state = disable_interrupts();

	config = _apic_read(APIC_LVT_TIMER) | APIC_LVT_MASKED; // mask the timer
	_apic_write(APIC_LVT_TIMER, config);

	_apic_write(APIC_INITIAL_TIMER_COUNT, 0); // zero out the timer

	config = _apic_read(APIC_LVT_TIMER) & ~APIC_LVT_MASKED; // unmask the timer
	_apic_write(APIC_LVT_TIMER, config);

	//TRACE_TIMER(("arch_smp_set_apic_timer: config 0x%lx, timeout %Ld, tics/sec %lu, tics %lu\n",
	//	config, relativeTimeout, sApicTicsPerSec, ticks));

	_apic_write(APIC_INITIAL_TIMER_COUNT, ticks); // start it up

	restore_interrupts(state);

	return B_OK;
}


static status_t
apic_clear_hardware_timer(void)
{
	cpu_status state;
	uint32 config;

	if (sApicPtr == NULL)
		return B_ERROR;

	state = disable_interrupts();

	config = _apic_read(APIC_LVT_TIMER) | APIC_LVT_MASKED; // mask the timer
	_apic_write(APIC_LVT_TIMER, config);

	_apic_write(APIC_INITIAL_TIMER_COUNT, 0); // zero out the timer

	restore_interrupts(state);
	return B_OK;
}


static status_t
apic_init(struct kernel_args *args)
{
	/* If we're in this method, arch_smp called the special init function.
	   Therefore, if we got here with sApicPtr NULL, there is no APIC! */
	if (sApicPtr == NULL)
		return B_ERROR;

	return B_OK;
}


status_t
apic_smp_init_timer(struct kernel_args *args, int32 cpu)
{
	uint32 config;

	if (args->arch_args.apic == NULL) {
		return B_ERROR;
	}

	/* This is in place of apic_preinit; if we're not already initialized,
	   register the interrupt handler and set the pointers */
	if (sApicPtr == NULL) {
		sApicPtr = (void *)args->arch_args.apic;
		sApicTicsPerSec = args->arch_args.apic_time_cv_factor;
		install_io_interrupt_handler(0xfb - ARCH_INTERRUPT_BASE, &apic_timer_interrupt, NULL, B_NO_LOCK_VECTOR);
	}

	/* setup timer */
	config = _apic_read(APIC_LVT_TIMER) & APIC_LVT_TIMER_MASK;
	config |= 0xfb | APIC_LVT_MASKED; // vector 0xfb, timer masked
	_apic_write(APIC_LVT_TIMER, config);

	_apic_write(APIC_INITIAL_TIMER_COUNT, 0); // zero out the clock

	config = _apic_read(APIC_TIMER_DIVIDE_CONFIG) & 0xfffffff0;
	config |= APIC_TIMER_DIVIDE_CONFIG_1; // clock division by 1
	_apic_write(APIC_TIMER_DIVIDE_CONFIG, config);

	return B_OK;
}
