/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include "condvar.h"

extern "C" {
#include <compat/sys/condvar.h>
#include <compat/sys/kernel.h>
}

#include <condition_variable.h>

#include <slab/Slab.h>

#include "device.h"


#define ticks_to_usecs(t) (1000000*(t) / hz)


extern "C" {
static object_cache* sConditionVariableCache;


status_t
init_condition_variables()
{
	sConditionVariableCache = create_object_cache("condition variables",
			sizeof (ConditionVariable), 0, NULL, NULL, NULL);
	if (sConditionVariableCache == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
uninit_condition_variables()
{
	delete_object_cache(sConditionVariableCache);
}

} /* extern "C" */


void
_cv_init(struct cv* conditionVariable, const char* description)
{
	conditionVariable->condVar =
			(ConditionVariable*)object_cache_alloc(sConditionVariableCache, 0);
	conditionVariable->condVar->Init(NULL, description);
}


void
_cv_wait_unlocked(struct cv* conditionVariable)
{
	conditionVariable->condVar->Wait();
}


int
_cv_timedwait_unlocked(struct cv* conditionVariable, int timeout)
{
	status_t status;

	status = conditionVariable->condVar->Wait(B_ABSOLUTE_TIMEOUT,
		ticks_to_usecs(timeout));

	if (status == B_OK)
		return ENOERR;
	else
		return EWOULDBLOCK;
}


void
_cv_signal(struct cv* conditionVariable)
{
	conditionVariable->condVar->NotifyOne();
}
