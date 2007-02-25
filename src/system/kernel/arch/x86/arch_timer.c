/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/stage2.h>
#include <kernel.h>

#include <arch/int.h>
#include <arch/cpu.h>

#include <console.h>
#include <debug.h>
#include <timer.h>
#include <int.h>

#include <arch/timer.h>

#include <arch/x86/smp_priv.h>
#include <arch/x86/timer.h>

#include "interrupts.h"


#define PIT_CLOCK_RATE 1193180
#define PIT_MAX_TIMER_INTERVAL (0xffff * 1000000ll / PIT_CLOCK_RATE)


static void
set_isa_hardware_timer(bigtime_t relative_timeout)
{
	uint16 next_event_clocks;

	if (relative_timeout <= 0)
		next_event_clocks = 2;			
	else if (relative_timeout < PIT_MAX_TIMER_INTERVAL)
		next_event_clocks = relative_timeout * PIT_CLOCK_RATE / 1000000;
	else
		next_event_clocks = 0xffff;

	out8(0x30, 0x43);		
	out8(next_event_clocks & 0xff, 0x40);
	out8((next_event_clocks >> 8) & 0xff, 0x40);

	arch_int_enable_io_interrupt(0);
}


static void
clear_isa_hardware_timer(void)
{
	// mask out the timer
	arch_int_disable_io_interrupt(0);
}


static int32
isa_timer_interrupt(void *data)
{
	return timer_interrupt();
}


int
apic_timer_interrupt(void)
{
	return timer_interrupt();
}


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	// try the apic timer first
	if (arch_smp_set_apic_timer(timeout) != B_OK)
		set_isa_hardware_timer(timeout);
}


void
arch_timer_clear_hardware_timer(void)
{
	if (arch_smp_clear_apic_timer() != B_OK)
		clear_isa_hardware_timer();
}


int
arch_init_timer(kernel_args *args)
{
	//dprintf("arch_init_timer: entry\n");

	install_io_interrupt_handler(0, &isa_timer_interrupt, NULL, 0);
	clear_isa_hardware_timer();

	// apic timer interrupt set up by smp code

	return 0;
}

