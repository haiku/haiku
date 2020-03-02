/*
 * Copyright 2007-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "condition_variable.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

// libroot
#include <user_thread.h>

// system
#include <syscalls.h>
#include <user_thread_defs.h>

#include <lock.h>
#include <util/AutoLock.h>

#include "thread.h"


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
	ConditionVariable*& GetLink(ConditionVariable* variable) const
		{ return variable->fNext; }
};

typedef BOpenHashTable<ConditionVariableHashDefinition> ConditionVariableHash;
static ConditionVariableHash sConditionVariableHash;
static mutex sConditionVariablesLock = MUTEX_INITIALIZER("condition variables");


// #pragma mark - ConditionVariableEntry


ConditionVariableEntry::ConditionVariableEntry()
	: fVariable(NULL)
{
}


ConditionVariableEntry::~ConditionVariableEntry()
{
	if (fVariable != NULL)
		_RemoveFromVariable();
}


bool
ConditionVariableEntry::Add(const void* object)
{
	ASSERT(object != NULL);

	fThread = get_current_thread();

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

	status_t error;
	while ((error = _kern_block_thread(flags, timeout)) == B_INTERRUPTED) {
	}

	_RemoveFromVariable();
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
ConditionVariableEntry::_AddToLockedVariable(ConditionVariable* variable)
{
	fThread = get_current_thread();
	fVariable = variable;
	fWaitStatus = STATUS_ADDED;
	fVariable->fEntries.Add(this);
}


void
ConditionVariableEntry::_RemoveFromVariable()
{
	MutexLocker _(sConditionVariablesLock);
	if (fVariable != NULL) {
		fVariable->fEntries.Remove(this);
		fVariable = NULL;
	}
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
ConditionVariable::Unpublish()
{
	ASSERT(fObject != NULL);

	MutexLocker locker(sConditionVariablesLock);

	sConditionVariableHash.RemoveUnchecked(this);
	fObject = NULL;
	fObjectType = NULL;

	if (!fEntries.IsEmpty())
		_NotifyLocked(true, B_ENTRY_NOT_FOUND);
}


void
ConditionVariable::Add(ConditionVariableEntry* entry)
{
	MutexLocker _(sConditionVariablesLock);
	entry->_AddToLockedVariable(this);
}


status_t
ConditionVariable::Wait(uint32 flags, bigtime_t timeout)
{
	ConditionVariableEntry entry;
	Add(&entry);
	return entry.Wait(flags, timeout);
}


void
ConditionVariable::_Notify(bool all, status_t result)
{
	MutexLocker locker(sConditionVariablesLock);

	if (!fEntries.IsEmpty()) {
		if (result > B_OK) {
			panic("tried to notify with invalid result %" B_PRId32 "\n",
				result);
			result = B_ERROR;
		}

		_NotifyLocked(all, result);
	}
}


/*! Called with interrupts disabled and the condition variable spinlock and
	thread lock held.
*/
void
ConditionVariable::_NotifyLocked(bool all, status_t result)
{
	// dequeue and wake up the blocked threads
	while (ConditionVariableEntry* entry = fEntries.RemoveHead()) {
		entry->fVariable = NULL;

		if (entry->fWaitStatus <= 0)
			continue;

		if (entry->fWaitStatus == STATUS_WAITING)
			_kern_unblock_thread(get_thread_id(entry->fThread), result);

		entry->fWaitStatus = result;

		if (!all)
			break;
	}
}


// #pragma mark -


void
condition_variable_init()
{
	status_t error = sConditionVariableHash.Init(kConditionVariableHashSize);
	if (error != B_OK) {
		panic("condition_variable_init(): Failed to init hash table: %s",
			strerror(error));
	}
}
