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
PrivateConditionVariableEntry::Add(const void* object)
{
	ASSERT(object != NULL);

	fThread = thread_get_current_thread();

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	fVariable = sConditionVariableHash.Lookup(object);
	if (fVariable) {
		fNext = fVariable->fEntries;
		fVariable->fEntries = this;
	}

	return (fVariable != NULL);
}


void
PrivateConditionVariableEntry::Wait()
{
	if (!are_interrupts_enabled()) {
		panic("wait_for_condition_variable_entry() called with interrupts "
			"disabled");
		return;
	}

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	if (fVariable != NULL) {
		struct thread* thread = thread_get_current_thread();
		thread->next_state = B_THREAD_WAITING;
		thread->condition_variable = fVariable;
		thread->sem.blocking = -1;

		GRAB_THREAD_LOCK();
		locker.Unlock();
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();
	}
}


void
PrivateConditionVariableEntry::Wait(const void* object)
{
	if (Add(object))
		Wait(object);
}


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
			entry = entry->fNext;
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
		entry = entry->fNext;
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
PrivateConditionVariable::Unpublish()
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
		_Notify();
}


void
PrivateConditionVariable::Notify()
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
		_Notify();
}


//! Called with interrupts disabled and the condition variable spinlock held.
void
PrivateConditionVariable::_Notify()
{
	// dequeue and wake up the blocked threads
	GRAB_THREAD_LOCK();

	while (PrivateConditionVariableEntry* entry = fEntries) {
		fEntries = entry->fNext;

		entry->fNext = NULL;
		entry->fVariable = NULL;

		entry->fThread->condition_variable = NULL;
		if (entry->fThread->state == B_THREAD_WAITING) {
			entry->fThread->state = B_THREAD_READY;
			scheduler_enqueue_in_run_queue(entry->fThread);
		}
	}

	RELEASE_THREAD_LOCK();
}


#if 0

void
publish_stack_condition_variable(condition_variable* variable,
	const void* object, const char* objectType)
{
	ASSERT(variable != NULL);
	ASSERT(object != NULL);

	variable->object = object;
	variable->object_type = objectType;
	variable->entries = NULL;

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	ASSERT_PRINT(sConditionVariableHash.Lookup(object) == NULL,
		"condition variable: %p\n", sConditionVariableHash.Lookup(object));

	sConditionVariableHash.InsertUnchecked(variable);
}


void
unpublish_condition_variable(const void* object)
{
	ASSERT(object != NULL);

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	condition_variable* variable = sConditionVariableHash.Lookup(object);
	condition_variable_entry* entries = NULL;
	if (variable) {
		sConditionVariableHash.RemoveUnchecked(variable);
		entries = variable->entries;
		variable->object = NULL;
		variable->object_type = NULL;
		variable->entries = NULL;
	} else {
		panic("No condition variable for %p\n", object);
	}

	if (entries)
		notify_condition_variable_entries(entries);
}


void
notify_condition_variable(const void* object)
{
	ASSERT(object != NULL);

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	condition_variable* variable = sConditionVariableHash.Lookup(object);
	condition_variable_entry* entries = NULL;
	if (variable) {
		entries = variable->entries;
		variable->entries = NULL;
	} else {
		panic("No condition variable for %p\n", object);
	}

	locker.Unlock();

	if (entries)
		notify_condition_variable_entries(entries);
}


void
wait_for_condition_variable(const void* object)
{
	condition_variable_entry entry;
	if (add_condition_variable_entry(object, &entry))
		wait_for_condition_variable_entry(&entry);
}


bool
add_condition_variable_entry(const void* object,
	condition_variable_entry* entry)
{
	ASSERT(object != NULL);
	ASSERT(entry != NULL);

	entry->thread = thread_get_current_thread();

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	condition_variable* variable = sConditionVariableHash.Lookup(object);
	if (variable) {
		entry->variable = variable;
		entry->next = variable->entries;
		variable->entries = entry;
	} else
		entry->variable = NULL;

	return (variable != NULL);
}


void
wait_for_condition_variable_entry(condition_variable_entry* entry)
{
	ASSERT(entry != NULL);

	if (!are_interrupts_enabled()) {
		panic("wait_for_condition_variable_entry() called with interrupts "
			"disabled");
		return;
	}

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	condition_variable* variable = entry->variable;
	if (variable != NULL) {
		struct thread* thread = thread_get_current_thread();
		thread->next_state = B_THREAD_WAITING;
		thread->condition_variable = variable;
		thread->sem.blocking = -1;
	}

	if (variable != NULL) {
		GRAB_THREAD_LOCK();
		locker.Unlock();
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();
	}
}
#endif


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

