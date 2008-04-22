/*
 * Copyright 2007-2008, Ingo Weinhold, bonefish@cs.tu-berlin.de.
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


static const int kConditionVariableHashSize = 512;


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
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);
	if (address == 0)
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

	if (variable != NULL) {
		variable->Dump();

		set_debug_variable("_cvar", (addr_t)variable);
		set_debug_variable("_object", (addr_t)variable->Object());

	} else
		kprintf("no condition variable at or with key %p\n", (void*)address);

	return 0;
}


// #pragma mark - PrivateConditionVariableEntry


bool
PrivateConditionVariableEntry::Add(const void* object, uint32 flags)
{
	ASSERT(object != NULL);

	fThread = thread_get_current_thread();

	InterruptsLocker _;
	SpinLocker locker(sConditionVariablesLock);

	fVariable = sConditionVariableHash.Lookup(object);

	struct thread* thread = thread_get_current_thread();
	thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_CONDITION_VARIABLE,
		fVariable);

	if (fVariable == NULL) {
		SpinLocker threadLocker(thread_spinlock);
		thread_unblock_locked(thread, B_ENTRY_NOT_FOUND);
		return false;
	}

	// add to the queue for the variable
	fVariable->fEntries.Add(this);

	return true;
}


status_t
PrivateConditionVariableEntry::Wait()
{
	if (!are_interrupts_enabled()) {
		panic("wait_for_condition_variable_entry() called with interrupts "
			"disabled");
		return B_ERROR;
	}

	InterruptsLocker _;

	SpinLocker threadLocker(thread_spinlock);
	status_t error = thread_block_locked(thread_get_current_thread());
	threadLocker.Unlock();

	SpinLocker locker(sConditionVariablesLock);

	// remove entry from variable, if not done yet
	if (fVariable != NULL) {
		fVariable->fEntries.Remove(this);
		fVariable = NULL;
	}

	return error;
}


status_t
PrivateConditionVariableEntry::Wait(const void* object, uint32 flags)
{
	if (Add(object, flags))
		return Wait();
	return B_ENTRY_NOT_FOUND;
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
		int count = variable->fEntries.Size();

		kprintf("%p  %p  %-20s %15d\n", variable, variable->fObject,
			variable->fObjectType, count);
	}
}


void
PrivateConditionVariable::Dump() const
{
	kprintf("condition variable %p\n", this);
	kprintf("  object:  %p (%s)\n", fObject, fObjectType);
	kprintf("  threads:");

	for (EntryList::ConstIterator it = fEntries.GetIterator();
		 PrivateConditionVariableEntry* entry = it.Next();) {
		kprintf(" %ld", entry->fThread->id);
	}
	kprintf("\n");
}


void
PrivateConditionVariable::Publish(const void* object, const char* objectType)
{
	ASSERT(object != NULL);

	fObject = object;
	fObjectType = objectType;
	new(&fEntries) EntryList;

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
	SpinLocker threadLocker(threadsLocked ? NULL : &thread_spinlock);
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

	if (!fEntries.IsEmpty())
		_Notify(true, B_ENTRY_NOT_FOUND);
}


void
PrivateConditionVariable::Notify(bool all, bool threadsLocked)
{
	ASSERT(fObject != NULL);

	InterruptsLocker _;
	SpinLocker threadLocker(threadsLocked ? NULL : &thread_spinlock);
	SpinLocker locker(sConditionVariablesLock);

#if KDEBUG
	PrivateConditionVariable* variable = sConditionVariableHash.Lookup(fObject);
	if (variable != this) {
		panic("Condition variable %p not published, found: %p", this, variable);
		return;
	}
#endif

	if (!fEntries.IsEmpty())
		_Notify(all, B_OK);
}


/*! Called with interrupts disabled and the condition variable spinlock and
	thread lock held.
*/
void
PrivateConditionVariable::_Notify(bool all, status_t result)
{
	// dequeue and wake up the blocked threads
	while (PrivateConditionVariableEntry* entry = fEntries.RemoveHead()) {
		entry->fVariable = NULL;

		thread_unblock_locked(entry->fThread, B_OK);

		if (!all)
			break;
	}
}


// #pragma mark -


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
