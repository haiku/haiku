/*
 * Copyright 2019-2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <boot/stage2.h>
#include <kernel.h>
#include <debug.h>
#include <interrupts.h>

#include <timer.h>
#include <arch/timer.h>
#include <arch/cpu.h>


static uint64 sTimerTicksUS;
static bigtime_t sTimerMaxInterval;
static uint64 sBootTime;

#define TIMER_DISABLED (0)
#define TIMER_ENABLE (1)
#define TIMER_IMASK (2)
#define TIMER_ISTATUS (4)

#define TIMER_IRQ 27


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	if (timeout > sTimerMaxInterval)
		timeout = sTimerMaxInterval;

	WRITE_SPECIALREG(CNTV_TVAL_EL0, timeout * sTimerTicksUS);
	WRITE_SPECIALREG(CNTV_CTL_EL0, TIMER_ENABLE);
}


void
arch_timer_clear_hardware_timer()
{
	WRITE_SPECIALREG(CNTV_CTL_EL0, TIMER_DISABLED);
}


int32
arch_timer_interrupt(void *data)
{
	WRITE_SPECIALREG(CNTV_CTL_EL0, TIMER_DISABLED);
	return timer_interrupt();
}


int
arch_init_timer(kernel_args *args)
{
	sTimerTicksUS = READ_SPECIALREG(CNTFRQ_EL0) / 1000000;
	sTimerMaxInterval = INT32_MAX / sTimerTicksUS;

	WRITE_SPECIALREG(CNTV_CTL_EL0, TIMER_DISABLED);
	install_io_interrupt_handler(TIMER_IRQ, &arch_timer_interrupt, NULL, 0);

	sBootTime = READ_SPECIALREG(CNTPCT_EL0);

	return B_OK;
}


bigtime_t
system_time(void)
{
	return (READ_SPECIALREG(CNTPCT_EL0) - sBootTime) / sTimerTicksUS;
}
