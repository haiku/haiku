/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <boot/stage2.h>
#include <arch/sh4/sh4.h>
#include <kernel/int.h>
#include <kernel/debug.h>
#include <kernel/timer.h>
#include <kernel/arch/cpu.h>

#define timer_rate 12500000
#define max_timer_interval ((bigtime_t)0xffffffff * 1000000 / timer_rate)
#define min_timer_interval ((bigtime_t)0x1000)

#define SYSTEM_TIME_TIMER_QUANTA 0xffffffff
static const bigtime_t system_time_timer_period = (bigtime_t)SYSTEM_TIME_TIMER_QUANTA * timer_rate / 1000000;
static volatile bigtime_t base_system_time = 0;

bigtime_t system_time()
{
	bigtime_t outtime;
	uint32 timer_val;
	int state = int_disable_interrupts();

restart:
	timer_val = SYSTEM_TIME_TIMER_QUANTA - *(uint32 *)TCNT1;

	if(*(uint16 *)TCR1 & 0x0100) {
		// underflow has occurred already
		outtime = base_system_time + system_time_timer_period + (bigtime_t)timer_val * 1000000 / timer_rate;
	} else {
		outtime = base_system_time + (bigtime_t)timer_val * 1000000 / timer_rate;
		if(*(uint16 *)TCR1 & 0x0100) {
			// an underflow must have happened since we checked last, redo
			goto restart;
		}
	}

	int_restore_interrupts(state);

	return outtime;
}

static void start_timer(int timer)
{
	uint8 old_val = *(uint8 *)TSTR;
	*(uint8 *)TSTR = old_val | (0x1 << timer % 3);
}

static void stop_timer(int timer)
{
	uint8 old_val = *(uint8 *)TSTR;
	*(uint8 *)TSTR = old_val & ~(0x1 << timer %3);
}

static void setup_timer(int timer, bigtime_t relative_timeout)
{
	uint32 timer_val;

	if(relative_timeout < min_timer_interval) {
		timer_val = min_timer_interval * timer_rate / 1000000;
	} else if(relative_timeout < max_timer_interval) {
		timer_val = relative_timeout * timer_rate / 1000000;
	} else {
		timer_val = 0xffffffff;
	}

	switch(timer) {
		case 0:
			*(uint16 *)TCR0 = 0x0020;
        		*(uint32 *)TCNT0 = timer_val;
        		*(uint32 *)TCOR0 = timer_val;
			break;
		case 1:
			*(uint16 *)TCR1 = 0x0020;
        		*(uint32 *)TCNT1 = timer_val;
        		*(uint32 *)TCOR1 = timer_val;
			break;
		case 2:
			*(uint16 *)TCR2 = 0x0020;
        		*(uint32 *)TCNT2 = timer_val;
        		*(uint32 *)TCOR2 = timer_val;
			break;
		default:
			break;
	}
}

void arch_timer_set_hardware_timer(bigtime_t timeout)
{
	stop_timer(0);
	setup_timer(0, timeout);
	start_timer(0);
}

void arch_timer_clear_hardware_timer()
{
	stop_timer(0);
}

static int timer_interrupt0()
{
	stop_timer(0);
	return timer_interrupt();
}

static void setup_system_time_timer()
{
	*(uint16 *)TCR1 = 0x0020;
	*(uint32 *)TCNT1 = SYSTEM_TIME_TIMER_QUANTA;
	*(uint32 *)TCOR1 = SYSTEM_TIME_TIMER_QUANTA;
}

static int timer_interrupt1()
{
	base_system_time += system_time_timer_period;
	*(uint16 *)TCR1 = 0x0020; // mask out the underflow bit
	return INT_NO_RESCHEDULE;
}

int arch_init_timer(kernel_args *ka)
{
	int i;
	uint8 old_val8;
	uint16 old_val16;
	dprintf("arch_init_timer: entry\n");

	int_set_io_interrupt_handler(32, &timer_interrupt0, NULL);
	int_set_io_interrupt_handler(33, &timer_interrupt1, NULL);

	// stop all of the timers
	*(uint8 *)TSTR = 0;

	// enable the interrupt on timer 0 & 1 & disable 2
	old_val16 = *(uint16 *)IPRA;
	*(uint16 *)IPRA = (old_val16 & 0x000f) | 0xef00;

	// start timer 1 counting forever
	base_system_time = 0;
	setup_system_time_timer();
	start_timer(1);

	return 0;
}

