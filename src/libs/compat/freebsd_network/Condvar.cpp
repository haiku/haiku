/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include "condvar.h"

extern "C" {
#include <compat/sys/condvar.h>
#include <compat/sys/kernel.h>
}

#include <new>

#include <condition_variable.h>
#include <slab/Slab.h>
#include <util/AutoLock.h>

#include "device.h"


#define ticks_to_usecs(t) (1000000*((bigtime_t)t) / hz)


static const int kConditionVariableHashSize = 32;


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
static spinlock sConditionVariablesLock;

extern "C" {

status_t
init_condition_variables()
{
	return sConditionVariableHash.Init(kConditionVariableHashSize);
}


void
uninit_condition_variables()
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariableHashDefinition definition;
	ConditionVariable* variable = sConditionVariableHash.Clear(true);

	while (variable != NULL) {
		ConditionVariable* next = definition.GetLink(variable);
		variable->Unpublish();
		delete variable;
		variable = next;
	}
}

} /* extern "C" */


void
_cv_init(const void* object, const char* description)
{
	ConditionVariable* conditionVariable 
		= new(std::nothrow) ConditionVariable();
	if (conditionVariable == NULL)
		panic("No memory left.");

	InterruptsSpinLocker _(sConditionVariablesLock);
	conditionVariable->Publish(object, description);
	sConditionVariableHash.Insert(conditionVariable);
}


void
_cv_destroy(const void* object)
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariable* conditionVariable 
		= sConditionVariableHash.Lookup(object);
	if (conditionVariable == NULL)
		return;

	conditionVariable->Unpublish();
	sConditionVariableHash.RemoveUnchecked(conditionVariable);
	delete conditionVariable;
}


void
_cv_wait_unlocked(const void* object)
{
	ConditionVariableEntry conditionVariableEntry;

	conditionVariableEntry.Wait(object);
}


int
_cv_timedwait_unlocked(const void* object, int timeout)
{
	ConditionVariableEntry conditionVariableEntry;
	
	status_t status = conditionVariableEntry.Wait(object, B_ABSOLUTE_TIMEOUT,
		ticks_to_usecs(timeout));

	if (status == B_OK)
		return ENOERR;
	else
		return EWOULDBLOCK;
}


void
_cv_signal(const void* object)
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariable* conditionVariable
		= sConditionVariableHash.Lookup(object);
	if (conditionVariable == NULL)
		return;

	conditionVariable->NotifyOne();
}


void
_cv_broadcast(const void* object)
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariable* conditionVariable
		= sConditionVariableHash.Lookup(object);
	if (conditionVariable == NULL)
		return;

	conditionVariable->NotifyAll();
}
