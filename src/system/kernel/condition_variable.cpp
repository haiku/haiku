/*
 * Copyright 2007-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#include <listeners.h>
#include <scheduling_analysis.h>
#include <thread.h>
#include <util/AutoLock.h>


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
static spinlock sConditionVariablesLock;


static int
list_condition_variables(int argc, char** argv)
{
	ConditionVariable::ListAll();
	return 0;
}


static int
dump_condition_variable(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);
	if (address == 0)
		return 0;

	ConditionVariable* variable = sConditionVariableHash.Lookup((void*)address);

	if (variable == NULL) {
		// It must be a direct pointer to a condition variable.
		variable = (ConditionVariable*)address;
	}

	if (variable != NULL) {
		variable->Dump();

		set_debug_variable("_cvar", (addr_t)variable);
		set_debug_variable("_object", (addr_t)variable->Object());

	} else
		kprintf("no condition variable at or with key %p\n", (void*)address);

	return 0;
}


// #pragma mark - ConditionVariableEntry


bool
ConditionVariableEntry::Add(const void* object)
{
	ASSERT(object != NULL);

	fThread = thread_get_current_thread();

	InterruptsSpinLocker _(sConditionVariablesLock);

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
	if (!are_interrupts_enabled()) {
		panic("ConditionVariableEntry::Wait() called with interrupts "
			"disabled, entry: %p, variable: %p", this, fVariable);
		return B_ERROR;
	}

	InterruptsLocker _;

	SpinLocker conditionLocker(sConditionVariablesLock);

	if (fVariable == NULL)
		return fWaitStatus;

	thread_prepare_to_block(fThread, flags,
		THREAD_BLOCK_TYPE_CONDITION_VARIABLE, fVariable);

	fWaitStatus = STATUS_WAITING;

	conditionLocker.Unlock();

	SpinLocker schedulerLocker(gSchedulerLock);

	status_t error;
	if ((flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0)
		error = thread_block_with_timeout_locked(flags, timeout);
	else
		error = thread_block_locked(thread_get_current_thread());

	schedulerLocker.Unlock();

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
	fThread = thread_get_current_thread();

	InterruptsSpinLocker _(sConditionVariablesLock);

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

	T_SCHEDULING_ANALYSIS(InitConditionVariable(this, object, objectType));
	NotifyWaitObjectListeners(&WaitObjectListener::ConditionVariableInitialized,
		this);
}


void
ConditionVariable::Publish(const void* object, const char* objectType)
{
	ASSERT(object != NULL);

	fObject = object;
	fObjectType = objectType;
	new(&fEntries) EntryList;

	T_SCHEDULING_ANALYSIS(InitConditionVariable(this, object, objectType));
	NotifyWaitObjectListeners(&WaitObjectListener::ConditionVariableInitialized,
		this);

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	ASSERT_PRINT(sConditionVariableHash.Lookup(object) == NULL,
		"condition variable: %p\n", sConditionVariableHash.Lookup(object));

	sConditionVariableHash.InsertUnchecked(this);
}


void
ConditionVariable::Unpublish(bool schedulerLocked)
{
	ASSERT(fObject != NULL);

	InterruptsLocker _;
	SpinLocker schedulerLocker(schedulerLocked ? NULL : &gSchedulerLock);
	SpinLocker locker(sConditionVariablesLock);

#if KDEBUG
	ConditionVariable* variable = sConditionVariableHash.Lookup(fObject);
	if (variable != this) {
		panic("Condition variable %p not published, found: %p", this, variable);
		return;
	}
#endif

	sConditionVariableHash.RemoveUnchecked(this);
	fObject = NULL;
	fObjectType = NULL;

	if (!fEntries.IsEmpty())
		_NotifyLocked(true, B_ENTRY_NOT_FOUND);
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


/*static*/ void
ConditionVariable::NotifyOne(const void* object, bool schedulerLocked,
	status_t result)
{
	InterruptsSpinLocker locker(sConditionVariablesLock);
	ConditionVariable* variable = sConditionVariableHash.Lookup(object);
	locker.Unlock();
	if (variable == NULL)
		return;

	variable->NotifyOne(schedulerLocked, result);
}


/*static*/ void
ConditionVariable::NotifyAll(const void* object, bool schedulerLocked,
	status_t result)
{
	InterruptsSpinLocker locker(sConditionVariablesLock);
	ConditionVariable* variable = sConditionVariableHash.Lookup(object);
	locker.Unlock();
	if (variable == NULL)
		return;

	variable->NotifyAll(schedulerLocked, result);
}


/*static*/ void
ConditionVariable::ListAll()
{
	kprintf("  variable      object (type)                waiting threads\n");
	kprintf("------------------------------------------------------------\n");
	ConditionVariableHash::Iterator it(&sConditionVariableHash);
	while (ConditionVariable* variable = it.Next()) {
		// count waiting threads
		int count = variable->fEntries.Count();

		kprintf("%p  %p  %-20s %15d\n", variable, variable->fObject,
			variable->fObjectType, count);
	}
}


void
ConditionVariable::Dump() const
{
	kprintf("condition variable %p\n", this);
	kprintf("  object:  %p (%s)\n", fObject, fObjectType);
	kprintf("  threads:");

	for (EntryList::ConstIterator it = fEntries.GetIterator();
		 ConditionVariableEntry* entry = it.Next();) {
		kprintf(" %ld", entry->fThread->id);
	}
	kprintf("\n");
}


void
ConditionVariable::_Notify(bool all, bool schedulerLocked, status_t result)
{
	InterruptsLocker _;
	SpinLocker schedulerLocker(schedulerLocked ? NULL : &gSchedulerLock);
	SpinLocker locker(sConditionVariablesLock);

	if (!fEntries.IsEmpty()) {
		if (result > B_OK) {
			panic("tried to notify with invalid result %ld\n", result);
			result = B_ERROR;
		}

		_NotifyLocked(all, result);
	}
}


/*! Called with interrupts disabled and the condition variable spinlock and
	scheduler lock held.
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
			thread_unblock_locked(entry->fThread, result);

		entry->fWaitStatus = result;

		if (!all)
			break;
	}
}


// #pragma mark -


void
condition_variable_init()
{
	new(&sConditionVariableHash) ConditionVariableHash;

	status_t error = sConditionVariableHash.Init(kConditionVariableHashSize);
	if (error != B_OK) {
		panic("condition_variable_init(): Failed to init hash table: %s",
			strerror(error));
	}

	add_debugger_command_etc("cvar", &dump_condition_variable,
		"Dump condition variable info",
		"<address>\n"
		"Prints info for the specified condition variable.\n"
		"  <address>  - Address of the condition variable or the object it is\n"
		"               associated with.\n", 0);
	add_debugger_command_etc("cvars", &list_condition_variables,
		"List condition variables",
		"\n"
		"Lists all existing condition variables\n", 0);
}
