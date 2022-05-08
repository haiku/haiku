/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include <compat/sys/systm.h>
#include <compat/sys/kernel.h>
#include <compat/sys/mutex.h>
#include <compat/sys/condvar.h>


int
msleep(void* identifier, struct mtx* mutex, int priority,
	const char* description, int timeout)
{
	struct cv channel;
	channel.condition.Publish(identifier, description);

	int status = cv_timedwait(&channel, mutex, timeout);

	channel.condition.Unpublish();
	return status;
}


void
wakeup(void* identifier)
{
	ConditionVariable::NotifyAll(identifier, B_OK);
}


void
wakeup_one(void* identifier)
{
	ConditionVariable::NotifyOne(identifier, B_OK);
}
