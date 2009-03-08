/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <condition_variable.h>

#include <new>

#include <lock.h>


#define STATUS_ADDED	1
#define STATUS_WAITING	2


struct condition_private {
	mutex		lock;
	sem_id		wait_sem;
	const void*	object;
};


status_t
ConditionVariableEntry::Wait(uint32 flags, bigtime_t timeout)
{
	if (fVariable == NULL)
		return fWaitStatus;

	condition_private* condition = (condition_private*)fVariable->fObject;
	fWaitStatus = STATUS_WAITING;

	status_t status;
	do {
		status = acquire_sem_etc(condition->wait_sem, 1, flags, timeout);
	} while (status == B_INTERRUPTED);

	mutex_lock(&condition->lock);

	// remove entry from variable, if not done yet
	if (fVariable != NULL) {
		fVariable->fEntries.Remove(this);
		fVariable = NULL;
	}

	mutex_unlock(&condition->lock);

	return status;
}


inline void
ConditionVariableEntry::AddToVariable(ConditionVariable* variable)
{
	fVariable = variable;
	fWaitStatus = STATUS_ADDED;
	fVariable->fEntries.Add(this);
}


//	#pragma mark -


void
ConditionVariable::Init(const void* object, const char* objectType)
{
	fObjectType = objectType;
	new(&fEntries) EntryList;

	condition_private* condition = new condition_private;
	mutex_init(&condition->lock, objectType);
	condition->wait_sem = create_sem(0, "condition variable wait");
	if (condition->wait_sem < B_OK)
		panic("cannot create condition variable.");

	condition->object = object;

	fObject = condition;
}


void
ConditionVariable::Add(ConditionVariableEntry* entry)
{
	entry->AddToVariable(this);
}


void
ConditionVariable::_Notify(bool all, bool threadsLocked)
{
	condition_private* condition = (condition_private*)fObject;
	mutex_lock(&condition->lock);

	uint32 count = 0;

	while (ConditionVariableEntry* entry = fEntries.RemoveHead()) {
		entry->fVariable = NULL;
		if (entry->fWaitStatus <= 0)
			continue;

		entry->fWaitStatus = B_OK;
		count++;

		if (!all)
			break;
	}

	release_sem_etc(condition->wait_sem, count, 0);
	mutex_unlock(&condition->lock);
}
