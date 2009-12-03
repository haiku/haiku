/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


extern "C" {
#include <compat/sys/condvar.h>
#include <compat/sys/kernel.h>
}

#include <new>

#include <condition_variable.h>

#include "condvar.h"
#include "device.h"


#define ticks_to_usecs(t) (1000000*((bigtime_t)t) / hz)


status_t
init_condition_variables()
{
	return B_OK;
}


void
uninit_condition_variables() {}


void
conditionPublish(struct cv* variable, const void* waitChannel, 
	const char* description)
{
	variable->waitChannel = waitChannel;
	variable->description = description;
	variable->condition = new(std::nothrow) ConditionVariable();
	variable->condition->Publish(waitChannel, description);
}


void
conditionUnpublish(const struct cv* variable)
{
	variable->condition->Unpublish();
	delete variable->condition;
}


int
conditionTimedWait(const struct cv* variable, const int timeout)
{
	ConditionVariableEntry variableEntry;

	status_t status = variableEntry.Wait(variable->waitChannel,
		B_RELATIVE_TIMEOUT, ticks_to_usecs(timeout));

	if (status != B_OK)
		status = EWOULDBLOCK;
	return status;
}


void
conditionWait(const struct cv* variable)
{
	ConditionVariableEntry variableEntry;

	variableEntry.Wait(variable->waitChannel);
}


void
conditionNotifyOne(const void* waitChannel)
{
	ConditionVariable::NotifyOne(waitChannel);
}


void
conditionNotifyAll(const void* waitChannel)
{
	ConditionVariable::NotifyAll(waitChannel);
}
