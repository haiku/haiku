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

	status_t status = variableEntry.Wait(waitChannel, B_RELATIVE_TIMEOUT,
		ticks_to_usecs(timeout));

	if (status != B_OK)
		status = EWOULDBLOCK;
	return status;
}


void
publishedConditionNotifyAll(const void* waitChannel)
{
	ConditionVariable::NotifyAll(waitChannel);
}
