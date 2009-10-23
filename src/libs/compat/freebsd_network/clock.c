/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "device.h"


int ticks;
struct net_timer hardclockTimer;


void hardclock(struct net_timer*, void*);


// TODO use the hardclock function in the compat layer actually.
status_t
init_clock()
{
	gStack->init_timer(&hardclockTimer, &hardclock, NULL);
	gStack->set_timer(&hardclockTimer, hz);

	return B_OK;
}


void
uninit_clock()
{
	gStack->cancel_timer(&hardclockTimer);
}


void
hardclock(struct net_timer* timer, void* argument)
{
	atomic_add((vint32*)&ticks, 1);
	gStack->set_timer(&hardclockTimer, hz);
}
