/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/mutex.h>
#include <compat/sys/condvar.h>

#include "condvar.h"


void cv_init(struct cv* conditionVariable, const char* description)
{
	_cv_init(conditionVariable, description);
}


void cv_signal(struct cv* conditionVariable)
{
	_cv_signal(conditionVariable);
}


int cv_timedwait(struct cv* conditionVariable, struct mtx* mutex, int timeout)
{
	int status;

	mtx_unlock(mutex);
	status = _cv_timedwait_unlocked(conditionVariable, timeout);
	mtx_lock(mutex);

	return status;
}


void cv_wait(struct cv* conditionVariable, struct mtx* mutex)
{
	mtx_unlock(mutex);
	_cv_wait_unlocked(conditionVariable);
	mtx_lock(mutex);
}
