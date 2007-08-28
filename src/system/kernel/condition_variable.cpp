/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include <condition_variable.h>

#include <new>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <int.h>
#include <thread.h>
#include <util/AutoLock.h>


static const int kConditionVariableHashSize = 64;


struct ConditionVariableHashDefinition {
	typedef const void* KeyType;
	typedef	PrivateConditionVariable ValueType;

	size_t HashKey(const void* key) const
		{ return (size_t)key; }
	size_t Hash(PrivateConditionVariable* variable) const
		{ return (size_t)variable->fObject; }
	bool Compare(const void* key, PrivateConditionVariable* variable) const
		{ return key == variable->fObject; }
	HashTableLink<PrivateConditionVariable>* GetLink(
			PrivateConditionVariable* variable) const
		{ return variable; }
};

typedef OpenHashTable<ConditionVariableHashDefinition> ConditionVariableHash;
static ConditionVariableHash sConditionVariableHash;
static spinlock sConditionVariablesLock;


static int
list_condition_variables(int argc, char** argv)
{
	PrivateConditionVariable::ListAll();
	return 0;
}


static int
dump_condition_variable(int argc, char** argv)
{
	if (argc < 2 || strlen(argv[1]) < 2
		|| argv[1][0] != '0'
		|| argv[1][1] != 'x') {
		kprintf("%s: invalid argument, pass address\n", argv[0]);
		return 0;
	}

	addr_t address = strtoul(argv[1], NULL, 0);
	if (address == NULL)
		return 0;

	PrivateConditionVariable* variable = sConditionVariableHash.Lookup(
		(void*)address);

	if (variable == NULL) {
		// It might be a direct pointer to a condition variable. Search the
		// hash.
		ConditionVariableHash::Iterator it(&sConditionVariableHash);
		while (PrivateConditionVariable* hashVariable = it.Next()) {
			if (hashVariable == (void*)address) {
				variable = hashVariable;
				break;
			}
		}
	}

	if (variable != NULL)
		variable->Dump();
	else
		kprintf("no condition variable at or with key %p\n", (void*)address);

	return 0;
}


// #pragma mark - PrivateConditionVariableEntry


bool
PrivateConditionVariableEntry::Add(const void* object,
	PrivateConditionVariableEntry* threadNext)
{
	ASSERT(object != NULL);

	fThread = thread_get_current_thread();
	fFlags = 0;
	fResult = B_OK;

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	// add to the list of entries for this thread
	fThreadNext = threadNext;
	if (threadNext) {
		fThreadPrevious = threadNext->fThreadPrevious;
		threadNext->fThreadPrevious = this;
	} else
		fThreadPrevious = NULL;

	// add to the queue for the variable
	fVariable = sConditionVariableHash.Lookup(object);
	if (fVariable) {
		fVariableNext = fVariable->fEntries;
		fVariable->fEntries = this;
	} else
		fResult = B_ENTRY_NOT_FOUND;

	return (fVariable != NULL);
}


status_t
PrivateConditionVariableEntry::Wait(uint32 flags)
{
	if (!are_interrupts_enabled()) {
		panic("wait_for_condition_variable_entry() called with interrupts "
			"disabled");
		return B_ERROR;
	}

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	// get first entry for this thread
	PrivateConditionVariableEntry* firstEntry = this;
	while (firstEntry->fThreadPrevious)
		firstEntry = firstEntry->fThreadPrevious;

	// check whether any entry has already been notified
	// (set the flags while at it)
	PrivateConditionVariableEntry* entry = firstEntry;
	while (entry) {
		if (entry->fVariable == NULL)
			return entry->fResult;

		entry->fFlags = flags;

		entry = entry->fThreadNext;
	}

	// all entries are unnotified -- wait
	struct thread* thread = thread_get_current_thread();
	thread->next_state = B_THREAD_WAITING;
	thread->condition_variable_entry = firstEntry;
	thread->sem.blocking = -1;

	GRAB_THREAD_LOCK();
	locker.Unlock();
	scheduler_reschedule();
	RELEASE_THREAD_LOCK();

	return firstEntry->fResult;
}


status_t
PrivateConditionVariableEntry::Wait(const void* object, uint32 flags)
{
	if (Add(object, NULL))
		return Wait(flags);
	return B_ENTRY_NOT_FOUND;
}


/*! Removes the entry from its variable.
	Interrupts must be disabled, sConditionVariablesLock must be held.
*/
void
PrivateConditionVariableEntry::_Remove()
{
	if (!fVariable)
		return;

	// fast path, if we're first in queue
	if (this == fVariable->fEntries) {
		fVariable->fEntries = fVariableNext;
		fVariableNext = NULL;
		return;
	}

	// we're not the first entry -- find our previous entry
	PrivateConditionVariableEntry* entry = fVariable->fEntries;
	while (entry->fVariableNext) {
		if (this == entry->fVariableNext) {
			entry->fVariableNext = fVariableNext;
			fVariableNext = NULL;
			return;
		}

		entry = entry->fVariableNext;
	}
}


class PrivateConditionVariableEntry::Private {
public:
	inline Private(PrivateConditionVariableEntry& entry)
		: fEntry(entry)
	{
	}

	inline uint32 Flags() const				{ return fEntry.fFlags; }
	inline void Remove() const				{ fEntry._Remove(); }
	inline void SetResult(status_t result)	{ fEntry.fResult = result; }

private:
	PrivateConditionVariableEntry&	fEntry;
};


// #pragma mark - PrivateConditionVariable


/*static*/ void
PrivateConditionVariable::ListAll()
{
	kprintf("  variable      object (type)                waiting threads\n");
	kprintf("------------------------------------------------------------\n");
	ConditionVariableHash::Iterator it(&sConditionVariableHash);
	while (PrivateConditionVariable* variable = it.Next()) {
		// count waiting threads
		int count = 0;
		PrivateConditionVariableEntry* entry = variable->fEntries;
		while (entry) {
			count++;
			entry = entry->fVariableNext;
		}

		kprintf("%p  %p  %-20s %15d\n", variable, variable->fObject,
			variable->fObjectType, count);
	}
}


void
PrivateConditionVariable::Dump()
{
	kprintf("condition variable %p\n", this);
	kprintf("  object:  %p (%s)\n", fObject, fObjectType);
	kprintf("  threads:");
	PrivateConditionVariableEntry* entry = fEntries;
	while (entry) {
		kprintf(" %ld", entry->fThread->id);
		entry = entry->fVariableNext;
	}
	kprintf("\n");
}


void
PrivateConditionVariable::Publish(const void* object, const char* objectType)
{
	ASSERT(object != NULL);

	fObject = object;
	fObjectType = objectType;
	fEntries = NULL;

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	ASSERT_PRINT(sConditionVariableHash.Lookup(object) == NULL,
		"condition variable: %p\n", sConditionVariableHash.Lookup(object));

	sConditionVariableHash.InsertUnchecked(this);
}


void
PrivateConditionVariable::Unpublish(bool threadsLocked)
{
	ASSERT(fObject != NULL);

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

#if KDEBUG
	PrivateConditionVariable* variable = sConditionVariableHash.Lookup(fObject);
	if (variable != this) {
		panic("Condition variable %p not published, found: %p", this, variable);
		return;
	}
#endif

	sConditionVariableHash.RemoveUnchecked(this);
	fObject = NULL;
	fObjectType = NULL;

	if (fEntries)
		_Notify(true, threadsLocked, B_ENTRY_NOT_FOUND);
}


void
PrivateConditionVariable::Notify(bool all, bool threadsLocked)
{
	ASSERT(fObject != NULL);

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

#if KDEBUG
	PrivateConditionVariable* variable = sConditionVariableHash.Lookup(fObject);
	if (variable != this) {
		panic("Condition variable %p not published, found: %p", this, variable);
		return;
	}
#endif

	if (fEntries)
		_Notify(all, threadsLocked, B_OK);
}


//! Called with interrupts disabled and the condition variable spinlock held.
void
PrivateConditionVariable::_Notify(bool all, bool threadsLocked, status_t result)
{
	// dequeue and wake up the blocked threads
	if (!threadsLocked)
		GRAB_THREAD_LOCK();

	while (PrivateConditionVariableEntry* entry = fEntries) {
		fEntries = entry->fVariableNext;
		struct thread* thread = entry->fThread;

		if (thread->condition_variable_entry != NULL)
			thread->condition_variable_entry->fResult = result;

		entry->fVariableNext = NULL;
		entry->fVariable = NULL;

		// remove other entries of this thread from their respective variables
		PrivateConditionVariableEntry* otherEntry = entry->fThreadPrevious;
		while (otherEntry) {
			otherEntry->_Remove();
			otherEntry = otherEntry->fThreadPrevious;
		}

		otherEntry = entry->fThreadNext;
		while (otherEntry) {
			otherEntry->_Remove();
			otherEntry = otherEntry->fThreadNext;
		}

		// wake up the thread
		thread->condition_variable_entry = NULL;
		if (thread->state == B_THREAD_WAITING) {
			thread->state = B_THREAD_READY;
			scheduler_enqueue_in_run_queue(thread);
		}

		if (!all)
			break;
	}

	if (!threadsLocked)
		RELEASE_THREAD_LOCK();
}


// #pragma mark -


/*!	Interrupts must be disabled, thread lock must be held. Note, that the thread
	lock may temporarily be released.
*/
status_t
condition_variable_interrupt_thread(struct thread* thread)
{
	if (thread == NULL || thread->state != B_THREAD_WAITING
		|| thread->condition_variable_entry == NULL) {
		return B_BAD_VALUE;
	}

	thread_id threadID = thread->id;

	// We also need the condition variable spin lock, so, in order to respect
	// the locking order, we must temporarily release the thread lock.
	RELEASE_THREAD_LOCK();
	SpinLocker locker(sConditionVariablesLock);
	GRAB_THREAD_LOCK();

	// re-get the thread and do the checks again
	thread = thread_get_thread_struct_locked(threadID);

	if (thread != NULL || thread->state != B_THREAD_WAITING
		|| thread->condition_variable_entry == NULL) {
		return B_BAD_VALUE;
	}

	PrivateConditionVariableEntry* entry = thread->condition_variable_entry;
	uint32 flags = PrivateConditionVariableEntry::Private(*entry).Flags();

	// interruptable?
	if ((flags & B_CAN_INTERRUPT) == 0
		&& ((flags & B_KILL_CAN_INTERRUPT) == 0
			|| (thread->sig_pending & KILL_SIGNALS) == 0)) {
		return B_NOT_ALLOWED;
	}

	PrivateConditionVariableEntry::Private(*entry).SetResult(B_INTERRUPTED);

	// remove all of the thread's entries from their variables
	while (entry) {
		PrivateConditionVariableEntry::Private(*entry).Remove();
		entry = entry->ThreadNext();
	}

	// wake up the thread
	thread->condition_variable_entry = NULL;
	thread->state = B_THREAD_READY;
	scheduler_enqueue_in_run_queue(thread);

	return B_OK;
}


void
condition_variable_init()
{
	new(&sConditionVariableHash) ConditionVariableHash(
		kConditionVariableHashSize);

	status_t error = sConditionVariableHash.InitCheck();
	if (error != B_OK) {
		panic("condition_variable_init(): Failed to init hash table: %s",
			strerror(error));
	}

	add_debugger_command("condition_variable", &dump_condition_variable,
		"Dump condition_variable");
	add_debugger_command("condition_variables", &list_condition_variables,
		"List condition variables");
}

