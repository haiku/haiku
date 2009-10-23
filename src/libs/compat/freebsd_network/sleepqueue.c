/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/systm.h>
#include <compat/sys/sleepqueue.h>


void
sleepq_add(void* identifier, struct mtx* mutex, const char* description,
	int flags, int queue)
{

}


int
sleepq_broadcast(void* identifier, int flags, int priority, int queue)
{
	return 0;
}


void
sleepq_lock(void* identifier)
{

}


void
sleepq_release(void* identifier)
{

}


void
sleepq_remove(struct thread* thread, void* identifier)
{

}


int
sleepq_timedwait(void* identifier, int priority)
{
	return 0;
}
