/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/systm.h>
#include <compat/sys/kernel.h>
#include <compat/sys/mutex.h>
#include <compat/sys/condvar.h>

#include "Condvar.h"


int
msleep(void* identifier, struct mtx* mutex, int priority,
	const char* description, int timeout)
{
	int status;
	struct cv sleep;

	conditionPublish(&sleep, identifier, description);

	// FreeBSD's msleep() does not allow the mutex to be NULL, but we
	// do, as we implement some other functions like tsleep() with it.
	if (mutex != NULL)
		mtx_unlock(mutex);

	status = publishedConditionTimedWait(identifier, timeout);

	if (mutex != NULL)
		mtx_lock(mutex);

	conditionUnpublish(&sleep);

	return status;
}


void
wakeup(void* identifier)
{
	publishedConditionNotifyAll(identifier);
}


void
wakeup_one(void* identifier)
{
	publishedConditionNotifyOne(identifier);
}
