/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/systm.h>
#include <compat/sys/kernel.h>
#include <compat/sys/sleepqueue.h>


#define	ticks_to_msecs(t)	(1000 * (t) / hz)


int
msleep(void* identifier, struct mtx* mutex, int priority,
	const char* description, int timeout)
{
	int status;

	// TODO can be removed once the sleepq functions are implemented.
	status = snooze(ticks_to_msecs(timeout));

	sleepq_lock(identifier);
	sleepq_add(identifier, mutex, description, 0, 0);
	sleepq_release(identifier);

	status = sleepq_timedwait(identifier, timeout);

	sleepq_lock(identifier);
	sleepq_remove(NULL, identifier);
	sleepq_release(identifier);

	return status;
}


void
wakeup(void* identifier)
{
	sleepq_lock(identifier);
	sleepq_broadcast(identifier, 0, 0, 0);
	sleepq_release(identifier);
}
