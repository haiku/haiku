/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "device.h"


#define CONVERT_HZ_TO_USECS(hertz) (1000000LL / (hertz))
#define FREEBSD_CLOCK_FREQUENCY_IN_HZ 1000


int ticks;
static timer sHardClockTimer;


/*!
 * Implementation of FreeBSD's hardclock timer.
 */
static status_t
hardClock(timer* hardClockTimer)
{
	atomic_add((vint32*)&ticks, 1);
	return B_OK;
}


/*!
 * Initialization of the hardclock timer.
 *
 * Note: We are not using the FreeBSD variable hz as the invocation frequency
 * as it is the case in FreeBSD's hardclock function. This is due to lower
 * system load. The hz (see compat/sys/kernel.h) variable in the compat layer is
 * set to 1000000 Hz, whereas it is usually set to 1000 Hz for FreeBSD.
 */
status_t
init_hard_clock()
{
	status_t status;

	ticks = 0;
	status = add_timer(&sHardClockTimer, hardClock,
		CONVERT_HZ_TO_USECS(FREEBSD_CLOCK_FREQUENCY_IN_HZ),
		B_PERIODIC_TIMER);

	return status;
}


void
uninit_hard_clock()
{
	cancel_timer(&sHardClockTimer);
}
