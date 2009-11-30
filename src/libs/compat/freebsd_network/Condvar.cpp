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


#define ticks_to_usecs(t) (1000000*(t) / hz)


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
	ConditionVariableHash::Iterator it = sConditionVariableHash.GetIterator();
	while (ConditionVariable* variable = it.Next()) {
		variable->Unpublish();
		free(variable);
	}
}

} /* extern "C" */


void
_cv_init(struct cv* conditionVariablePointer, const char* description)
{
	ConditionVariable* conditionVariable 
		= new(std::nothrow) ConditionVariable();
	if (conditionVariable == NULL)
		panic("No memory left.");
	conditionVariablePointer->cv_waiters = 0;
	conditionVariable->Init(conditionVariablePointer, description);
	InterruptsSpinLocker _(sConditionVariablesLock);
	sConditionVariableHash.Insert(conditionVariable);
}


void
_cv_wait_unlocked(struct cv* conditionVariablePointer)
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariable* conditionVariable
		= sConditionVariableHash.Lookup(conditionVariablePointer);
	conditionVariablePointer->cv_waiters++;
	conditionVariable->Wait();
}


int
_cv_timedwait_unlocked(struct cv* conditionVariablePointer, int timeout)
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariable* conditionVariable
		= sConditionVariableHash.Lookup(conditionVariablePointer);
	conditionVariablePointer->cv_waiters++;
	status_t status = conditionVariable->Wait(B_ABSOLUTE_TIMEOUT,
		ticks_to_usecs(timeout));

	if (status == B_OK)
		return ENOERR;
	else
		return EWOULDBLOCK;
}


void
_cv_signal(struct cv* conditionVariablePointer)
{
	InterruptsSpinLocker _(sConditionVariablesLock);
	ConditionVariable* conditionVariable
		= sConditionVariableHash.Lookup(conditionVariablePointer);
	if (conditionVariablePointer->cv_waiters > 0)
		conditionVariablePointer->cv_waiters--;
	conditionVariable->NotifyOne();
}
