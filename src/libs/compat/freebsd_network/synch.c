/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/systm.h>
#include <compat/sys/kernel.h>
#include <compat/sys/mutex.h>

#include "condvar.h"


static int sPauseWaitChannel;


int
msleep(void* identifier, struct mtx* mutex, int priority,
	const char* description, int timeout)
{
	int status;

	_cv_init(identifier, description);
	
	mtx_unlock(mutex);
	status = _cv_timedwait_unlocked(identifier, timeout);
	mtx_lock(mutex);
	
	_cv_destroy(identifier);
	return status;
}


void
wakeup(void* identifier)
{
	_cv_broadcast(identifier);
}


int
_pause(const char* waitMessage, int timeout)
{

	KASSERT(timeout != 0, ("pause: timeout required"));
	return tsleep(&sPauseWaitChannel, 0, waitMessage, timeout);
}
