/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_timer_generic.h"


#define TIMER_IRQ 30


static uint32_t
get_counter_freq(void)
{
    uint32_t freq;
    asm volatile ("MRC p15, 0, %0, c14, c0, 0": "=r" (freq));
    return freq;
}


static uint64_t
get_counter(void)
{
    uint32_t counter_low;
    uint32_t counter_high;

    asm volatile ("ISB\n"
        "MRRC p15, 0, %0, %1, c14"
        : "=r" (counter_low), "=r" (counter_high));
    return ((uint64_t)counter_high << 32) | counter_low;
}


void
ARMGenericTimer::SetTimeout(bigtime_t timeout)
{
	uint32_t timeout_ticks = timeout * fTimerFrequency / 1000000;
	asm volatile ("ISB\n"
		"MCR p15, 0, %0, c14, c2, 0"
		: : "r"(timeout_ticks));

	uint32_t timer_ctl = 1;
	asm volatile ("MCR p15, 0, %0, c14, c2, 1"
		: : "r"(timer_ctl));
}


void
ARMGenericTimer::Clear()
{
	uint32_t timer_ctl = 2;
	asm volatile ("MCR p15, 0, %0, c14, c2, 1"
		: : "r"(timer_ctl));
}


bigtime_t
ARMGenericTimer::Time()
{
	return get_counter() / fTimerFrequency;
}


int32
ARMGenericTimer::_InterruptWrapper(void *data)
{
	return ((ARMGenericTimer*)data)->HandleInterrupt();
}


int32
ARMGenericTimer::HandleInterrupt()
{
	return timer_interrupt();
}


ARMGenericTimer::ARMGenericTimer()
: HardwareTimer()
{
	Clear();

	reserve_io_interrupt_vectors(1, TIMER_IRQ, INTERRUPT_TYPE_IRQ);
	install_io_interrupt_handler(TIMER_IRQ,
		&ARMGenericTimer::_InterruptWrapper, this, 0);

	fTimerFrequency = get_counter_freq();
}


bool
ARMGenericTimer::IsAvailable()
{
	uint32_t pfr1;
	asm volatile("MRC p15, 0, %0, c0, c1, 1": "=r" (pfr1));
	return (pfr1 & 0x000F0000) == 0x00010000;
}
