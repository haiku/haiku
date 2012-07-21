/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <compat/sys/kernel.h>


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
 * Initialization of the hardclock timer which ticks according to hz defined in
 * compat/sys/kernel.h.
 */
status_t
init_hard_clock()
{
	ticks = 0;
	return add_timer(&sHardClockTimer, hardClock, ticks_to_usecs(1),
		B_PERIODIC_TIMER);
}


void
uninit_hard_clock()
{
	cancel_timer(&sHardClockTimer);
}
