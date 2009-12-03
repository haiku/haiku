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

	mtx_unlock(mutex);
	status = publishedConditionTimedWait(identifier, timeout);
	mtx_lock(mutex);

	conditionUnpublish(&sleep);

	return status;
}


void
wakeup(void* identifier)
{
	publishedConditionNotifyAll(identifier);
}


int
_pause(const char* waitMessage, int timeout)
{
	int waitChannel;
	KASSERT(timeout != 0, ("pause: timeout required"));
	return tsleep(&waitChannel, 0, waitMessage, timeout);
}
