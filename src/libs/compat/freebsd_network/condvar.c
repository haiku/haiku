/*
 * Copyright 2009 Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/mutex.h>
#include <compat/sys/condvar.h>

#include "condvar.h"


void cv_init(struct cv* conditionVariable, const char* description)
{
	conditionVariable->description = description;
}


void cv_signal(struct cv* conditionVariable)
{
	_cv_signal(conditionVariable);
}


int cv_timedwait(struct cv* conditionVariable, struct mtx* mutex, int timeout)
{
	int status;

	_cv_init(conditionVariable, conditionVariable->description);

	mtx_unlock(mutex);
	status = _cv_timedwait_unlocked(conditionVariable, timeout);
	mtx_lock(mutex);

	_cv_destroy(conditionVariable);

	return status;
}


void cv_wait(struct cv* conditionVariable, struct mtx* mutex)
{
	_cv_init(conditionVariable, conditionVariable->description);

	mtx_unlock(mutex);
	_cv_wait_unlocked(conditionVariable);
	mtx_lock(mutex);

	_cv_destroy(conditionVariable);
}
