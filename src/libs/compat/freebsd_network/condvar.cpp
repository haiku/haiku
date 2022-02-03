/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include <compat/sys/mutex.h>
#include <compat/sys/condvar.h>
#include <compat/sys/kernel.h>


void
cv_init(struct cv* variable, const char* description)
{
	variable->condition.Init(NULL, description);
}


void
cv_signal(struct cv* variable)
{
	variable->condition.NotifyOne();
}


int
cv_timedwait(struct cv* variable, struct mtx* mutex, int timeout)
{
	int status;

	const uint32 flags = timeout ? B_RELATIVE_TIMEOUT : 0;
	const bigtime_t bigtimeout = TICKS_2_USEC(timeout);

	if (mutex->type == MTX_RECURSE) {
		// Special case: let the ConditionVariable handle switching recursive locks.
		status = variable->condition.Wait(&mutex->u.recursive,
			flags, bigtimeout);
		return status;
	}

	ConditionVariableEntry entry;
	variable->condition.Add(&entry);

	mtx_unlock(mutex);

	status = entry.Wait(flags, bigtimeout);

	mtx_lock(mutex);
	return status;
}


void
cv_wait(struct cv* variable, struct mtx* mutex)
{
	cv_timedwait(variable, mutex, 0);
}
