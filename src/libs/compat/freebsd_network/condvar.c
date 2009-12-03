/*
 * Copyright 2009 Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/mutex.h>
#include <compat/sys/condvar.h>

#include "condvar.h"


void cv_init(struct cv* variable, const char* description)
{
	conditionPublish(variable, variable, description);
}


void cv_destroy(struct cv* variable)
{
	conditionUnpublish(variable);
}


void cv_signal(struct cv* variable)
{
	conditionNotifyOne(variable);
}


int cv_timedwait(struct cv* variable, struct mtx* mutex, int timeout)
{
	int status;

	mtx_unlock(mutex);
	status = conditionTimedWait(variable, timeout);
	mtx_lock(mutex);

	return status;
}


void cv_wait(struct cv* variable, struct mtx* mutex)
{
	mtx_unlock(mutex);
	conditionWait(variable);
	mtx_lock(mutex);
}
