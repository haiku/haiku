/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


extern "C" {
#include <compat/sys/condvar.h>
#include <compat/sys/kernel.h>
}

#include "Condvar.h"


void
conditionInit(struct cv* variable, const char* description)
{
	variable->condition.Init(variable, description);
}


void
conditionPublish(struct cv* variable, const void* waitChannel,
	const char* description)
{
	variable->condition.Publish(waitChannel, description);
}


void
conditionUnpublish(struct cv* variable)
{
	variable->condition.Unpublish();
}


int
conditionTimedWait(struct cv* variable, const int timeout)
{
	status_t status = variable->condition.Wait(B_RELATIVE_TIMEOUT,
		ticks_to_usecs(timeout));

	if (status != B_OK)
		status = EWOULDBLOCK;
	return status;
}


void
conditionWait(struct cv* variable)
{
	variable->condition.Wait();
}


void
conditionNotifyOne(struct cv* variable)
{
	variable->condition.NotifyOne();
}


int
publishedConditionTimedWait(const void* waitChannel, const int timeout)
{
	ConditionVariableEntry variableEntry;
	bigtime_t usecs = ticks_to_usecs(timeout);

	// FreeBSD has a condition-variable scheduling system with different
	// scheduling semantics than ours does. As a result, it seems there are
	// some scenarios which work fine under FreeBSD but race into a deadlock
	// on Haiku. To avoid this, turn unlimited timeouts into 1sec ones.
	status_t status = variableEntry.Wait(waitChannel, B_RELATIVE_TIMEOUT,
		usecs > 0 ? usecs : 1000 * 1000);

	if (status != B_OK)
		status = EWOULDBLOCK;
	return status;
}


void
publishedConditionNotifyAll(const void* waitChannel)
{
	ConditionVariable::NotifyAll(waitChannel, B_OK);
}


void
publishedConditionNotifyOne(const void* waitChannel)
{
	ConditionVariable::NotifyOne(waitChannel, B_OK);
}
