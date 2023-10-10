/*
 * Copyright 2022-2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

extern "C" {
#include <compat/sys/mutex.h>
#include <compat/sys/kernel.h>
#include <compat/sys/condvar.h>
}

#include <condition_variable.h>


static_assert(sizeof(cv::condition) == sizeof(ConditionVariable));


void
cv_init(struct cv* variable, const char* description)
{
	__cv_ConditionVariable(variable)->Init(NULL, description);
}


void
cv_destroy(struct cv* variable)
{
	__cv_ConditionVariable(variable)->NotifyAll();

	// Nothing else to do.
}


void
cv_signal(struct cv* variable)
{
	__cv_ConditionVariable(variable)->NotifyOne();
}


int
cv_timedwait(struct cv* variable, struct mtx* mutex, int timeout)
{
	ConditionVariable* condition = __cv_ConditionVariable(variable);

	const uint32 flags = timeout ? B_RELATIVE_TIMEOUT : 0;
	const bigtime_t bigtimeout = TICKS_2_USEC(timeout);
	int status;

	if (mutex->type == MTX_RECURSE) {
		// Special case: let the ConditionVariable handle switching recursive locks.
		status = condition->Wait(&mutex->u.recursive,
			flags, bigtimeout);
		return status;
	}

	ConditionVariableEntry entry;
	condition->Add(&entry);

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
