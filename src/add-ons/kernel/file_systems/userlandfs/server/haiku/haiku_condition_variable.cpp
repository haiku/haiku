/*
 * Copyright 2007-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "haiku_condition_variable.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <KernelExport.h>

// libroot
#include <user_thread.h>

// system
#include <syscalls.h>
#include <user_thread_defs.h>

#include "haiku_lock.h"


#define STATUS_ADDED	1
#define STATUS_WAITING	2


static const int kConditionVariableHashSize = 512;


struct ConditionVariableHashDefinition {
	typedef const void* KeyType;
	typedef	ConditionVariable ValueType;

	size_t HashKey(const void* key) const
		{ return (size_t)key; }
	size_t Hash(ConditionVariable* variable) const
		{ return (size_t)variable->fObject; }
	bool Compare(const void* key, ConditionVariable* variable) const
		{ return key == variable->fObject; }
	HashTableLink<ConditionVariable>* GetLink(ConditionVariable* variable) const
		{ return variable; }
};

typedef OpenHashTable<ConditionVariableHashDefinition> ConditionVariableHash;
static ConditionVariableHash sConditionVariableHash;
static mutex sConditionVariablesLock;
static mutex sThreadsLock;


// #pragma mark - ConditionVariableEntry


bool
ConditionVariableEntry::Add(const void* object)
{
	ASSERT(object != NULL);

	fThread = find_thread(NULL);

	MutexLocker _(sConditionVariablesLock);

	fVariable = sConditionVariableHash.Lookup(object);

	if (fVariable == NULL) {
		fWaitStatus = B_ENTRY_NOT_FOUND;
		return false;
	}

	fWaitStatus = STATUS_ADDED;
	fVariable->fEntries.Add(this);

	return true;
}


status_t
ConditionVariableEntry::Wait(uint32 flags, bigtime_t timeout)
{
	MutexLocker conditionLocker(sConditionVariablesLock);

	if (fVariable == NULL)
		return fWaitStatus;

	user_thread* userThread = get_user_thread();

	userThread->wait_status = 1;
	fWaitStatus = STATUS_WAITING;

	conditionLocker.Unlock();

	MutexLocker threadLocker(sThreadsLock);

	status_t error;
	if ((flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0)
		error = _kern_block_thread(flags, timeout);
	else
		error = _kern_block_thread(0, 0);
	threadLocker.Unlock();

	conditionLocker.Lock();

	// remove entry from variable, if not done yet
	if (fVariable != NULL) {
		fVariable->fEntries.Remove(this);
		fVariable = NULL;
	}

	return error;
}


status_t
ConditionVariableEntry::Wait(const void* object, uint32 flags,
	bigtime_t timeout)
{
	if (Add(object))
		return Wait(flags, timeout);
	return B_ENTRY_NOT_FOUND;
}


inline void
ConditionVariableEntry::AddToVariable(ConditionVariable* variable)
{
	fThread = find_thread(NULL);

	MutexLocker _(sConditionVariablesLock);

	fVariable = variable;
	fWaitStatus = STATUS_ADDED;
	fVariable->fEntries.Add(this);
}


// #pragma mark - ConditionVariable


/*!	Initialization method for anonymous (unpublished) condition variables.
*/
void
ConditionVariable::Init(const void* object, const char* objectType)
{
	fObject = object;
	fObjectType = objectType;
	new(&fEntries) EntryList;
}


void
ConditionVariable::Publish(const void* object, const char* objectType)
{
	ASSERT(object != NULL);

	fObject = object;
	fObjectType = objectType;
	new(&fEntries) EntryList;

	MutexLocker locker(sConditionVariablesLock);

	ASSERT(sConditionVariableHash.Lookup(object) == NULL);

	sConditionVariableHash.InsertUnchecked(this);
}


void
ConditionVariable::Unpublish(bool threadsLocked)
{
	ASSERT(fObject != NULL);

	MutexLocker threadLocker(threadsLocked ? NULL : &sThreadsLock);
	MutexLocker locker(sConditionVariablesLock);

	sConditionVariableHash.RemoveUnchecked(this);
	fObject = NULL;
	fObjectType = NULL;

	if (!fEntries.IsEmpty())
		_NotifyChecked(true, B_ENTRY_NOT_FOUND);
}


void
ConditionVariable::Add(ConditionVariableEntry* entry)
{
	entry->AddToVariable(this);
}


status_t
ConditionVariable::Wait(uint32 flags, bigtime_t timeout)
{
	ConditionVariableEntry entry;
	Add(&entry);
	return entry.Wait(flags, timeout);
}


void
ConditionVariable::_Notify(bool all, bool threadsLocked)
{
	MutexLocker threadLocker(threadsLocked ? NULL : &sThreadsLock);
	MutexLocker locker(sConditionVariablesLock);

	if (!fEntries.IsEmpty())
		_NotifyChecked(all, B_OK);
}


/*! Called with interrupts disabled and the condition variable spinlock and
	thread lock held.
*/
void
ConditionVariable::_NotifyChecked(bool all, status_t result)
{
	// dequeue and wake up the blocked threads
	while (ConditionVariableEntry* entry = fEntries.RemoveHead()) {
		entry->fVariable = NULL;

		if (entry->fWaitStatus <= 0)
			continue;

		if (entry->fWaitStatus == STATUS_WAITING)
			_kern_unblock_thread(entry->fThread, result);

		entry->fWaitStatus = result;

		if (!all)
			break;
	}
}


// #pragma mark -


status_t
condition_variable_init()
{
	mutex_init(&sConditionVariablesLock, "condition variables");
	mutex_init(&sThreadsLock, "threads");

	new(&sConditionVariableHash) ConditionVariableHash;

	status_t error = sConditionVariableHash.Init(kConditionVariableHashSize);
	if (error != B_OK) {
		panic("condition_variable_init(): Failed to init hash table: %s",
			strerror(error));
	}

	return error;
}
