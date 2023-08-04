/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2015, Hamish Morrison, hamishm53@gmail.com.
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <user_mutex.h>
#include <user_mutex_defs.h>

#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <smp.h>
#include <syscall_restart.h>
#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <util/OpenHashTable.h>
#include <vm/vm.h>
#include <vm/VMArea.h>
#include <arch/generic/user_memory.h>


/*! One UserMutexEntry corresponds to one mutex address.
 *
 * The mutex's "waiting" state is controlled by the rw_lock: a waiter acquires
 * a "read" lock before initiating a wait, and an unblocker acquires a "write"
 * lock. That way, unblockers can be sure that no waiters will start waiting
 * during unblock, and they can thus safely (without races) unset WAITING.
 */
struct UserMutexEntry {
	generic_addr_t		address;
	UserMutexEntry*		hash_next;
	int32				ref_count;

	rw_lock				lock;
	ConditionVariable	condition;
};

struct UserMutexHashDefinition {
	typedef generic_addr_t	KeyType;
	typedef UserMutexEntry	ValueType;

	size_t HashKey(generic_addr_t key) const
	{
		return key >> 2;
	}

	size_t Hash(const UserMutexEntry* value) const
	{
		return HashKey(value->address);
	}

	bool Compare(generic_addr_t key, const UserMutexEntry* value) const
	{
		return value->address == key;
	}

	UserMutexEntry*& GetLink(UserMutexEntry* value) const
	{
		return value->hash_next;
	}
};

typedef BOpenHashTable<UserMutexHashDefinition> UserMutexTable;


struct user_mutex_context {
	UserMutexTable table;
	rw_lock lock;
};
static user_mutex_context sSharedUserMutexContext;
static const char* kUserMutexEntryType = "umtx entry";


// #pragma mark - user atomics


static int32
user_atomic_or(int32* value, int32 orValue, bool isWired)
{
	int32 result;
	if (isWired) {
		set_ac();
		result = atomic_or(value, orValue);
		clear_ac();
		return result;
	}

	return user_access([=, &result] {
		result = atomic_or(value, orValue);
	}) ? result : INT32_MIN;
}


static int32
user_atomic_and(int32* value, int32 andValue, bool isWired)
{
	int32 result;
	if (isWired) {
		set_ac();
		result = atomic_and(value, andValue);
		clear_ac();
		return result;
	}

	return user_access([=, &result] {
		result = atomic_and(value, andValue);
	}) ? result : INT32_MIN;
}


static int32
user_atomic_get(int32* value, bool isWired)
{
	int32 result;
	if (isWired) {
		set_ac();
		result = atomic_get(value);
		clear_ac();
		return result;
	}

	return user_access([=, &result] {
		result = atomic_get(value);
	}) ? result : INT32_MIN;
}


static int32
user_atomic_test_and_set(int32* value, int32 newValue, int32 testAgainst,
	bool isWired)
{
	int32 result;
	if (isWired) {
		set_ac();
		result = atomic_test_and_set(value, newValue, testAgainst);
		clear_ac();
		return result;
	}

	return user_access([=, &result] {
		result = atomic_test_and_set(value, newValue, testAgainst);
	}) ? result : INT32_MIN;
}


// #pragma mark - user mutex context


static int
dump_user_mutex(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	addr_t threadID = parse_expression(argv[1]);
	if (threadID == 0)
		return 0;

	Thread* thread = Thread::GetDebug(threadID);
	if (thread == NULL) {
		kprintf("no such thread\n");
		return 0;
	}

	if (thread->wait.type != THREAD_BLOCK_TYPE_CONDITION_VARIABLE) {
		kprintf("thread is not blocked on cvar (thus not user_mutex)\n");
		return 0;
	}

	ConditionVariable* variable = (ConditionVariable*)thread->wait.object;
	if (variable->ObjectType() != kUserMutexEntryType) {
		kprintf("thread is not blocked on user_mutex\n");
		return 0;
	}

	UserMutexEntry* entry = (UserMutexEntry*)variable->Object();

	const bool physical = (sSharedUserMutexContext.table.Lookup(entry->address) == entry);
	kprintf("user mutex entry %p\n", entry);
	kprintf("  address:  0x%" B_PRIxPHYSADDR " (%s)\n", entry->address,
		physical ? "physical" : "virtual");
	kprintf("  refcount: %" B_PRId32 "\n", entry->ref_count);
	kprintf("  lock:     %p\n", &entry->lock);

	int32 mutex = 0;
	status_t status = B_ERROR;
	if (!physical) {
		status = debug_memcpy(thread->team->id, &mutex,
			(void*)entry->address, sizeof(mutex));
	}

	if (status == B_OK)
		kprintf("  mutex:    0x%" B_PRIx32 "\n", mutex);

	entry->condition.Dump();

	return 0;
}


void
user_mutex_init()
{
	sSharedUserMutexContext.lock = RW_LOCK_INITIALIZER("shared user mutex table");
	if (sSharedUserMutexContext.table.Init() != B_OK)
		panic("user_mutex_init(): Failed to init table!");

	add_debugger_command_etc("user_mutex", &dump_user_mutex,
		"Dump user-mutex info",
		"<thread>\n"
		"Prints info about the user-mutex a thread is blocked on.\n"
		"  <thread>  - Thread ID that is blocked on a user mutex\n", 0);
}


struct user_mutex_context*
get_team_user_mutex_context()
{
	struct user_mutex_context* context =
		thread_get_current_thread()->team->user_mutex_context;
	if (context != NULL)
		return context;

	Team* team = thread_get_current_thread()->team;
	TeamLocker teamLocker(team);
	if (team->user_mutex_context != NULL)
		return team->user_mutex_context;

	context = new(std::nothrow) user_mutex_context;
	if (context == NULL)
		return NULL;

	context->lock = RW_LOCK_INITIALIZER("user mutex table");
	if (context->table.Init() != B_OK) {
		delete context;
		return NULL;
	}

	team->user_mutex_context = context;
	return context;
}


void
delete_user_mutex_context(struct user_mutex_context* context)
{
	if (context == NULL)
		return;

	// This should be empty at this point in team destruction.
	ASSERT(context->table.IsEmpty());
	delete context;
}


static UserMutexEntry*
get_user_mutex_entry(struct user_mutex_context* context,
	generic_addr_t address, bool noInsert = false, bool alreadyLocked = false)
{
	ReadLocker tableReadLocker;
	if (!alreadyLocked)
		tableReadLocker.SetTo(context->lock, false);

	UserMutexEntry* entry = context->table.Lookup(address);
	if (entry != NULL) {
		atomic_add(&entry->ref_count, 1);
		return entry;
	} else if (noInsert)
		return entry;

	tableReadLocker.Unlock();
	WriteLocker tableWriteLocker(context->lock);

	entry = context->table.Lookup(address);
	if (entry != NULL) {
		atomic_add(&entry->ref_count, 1);
		return entry;
	}

	entry = new(std::nothrow) UserMutexEntry;
	if (entry == NULL)
		return entry;

	entry->address = address;
	entry->ref_count = 1;
	rw_lock_init(&entry->lock, "UserMutexEntry lock");
	entry->condition.Init(entry, kUserMutexEntryType);

	context->table.Insert(entry);
	return entry;
}


static void
put_user_mutex_entry(struct user_mutex_context* context, UserMutexEntry* entry)
{
	if (entry == NULL)
		return;

	const generic_addr_t address = entry->address;
	if (atomic_add(&entry->ref_count, -1) != 1)
		return;

	WriteLocker tableWriteLocker(context->lock);

	// Was it removed & deleted while we were waiting for the lock?
	if (context->table.Lookup(address) != entry)
		return;

	// Or did someone else acquire a reference to it?
	if (atomic_get(&entry->ref_count) > 0)
		return;

	context->table.Remove(entry);
	tableWriteLocker.Unlock();

	rw_lock_destroy(&entry->lock);
	delete entry;
}


static status_t
user_mutex_wait_locked(UserMutexEntry* entry,
	uint32 flags, bigtime_t timeout, ReadLocker& locker)
{
	ConditionVariableEntry waiter;
	entry->condition.Add(&waiter);
	locker.Unlock();

	return waiter.Wait(flags, timeout);
}


static bool
user_mutex_prepare_to_lock(UserMutexEntry* entry, int32* mutex, bool isWired)
{
	ASSERT_READ_LOCKED_RW_LOCK(&entry->lock);

	int32 oldValue = user_atomic_or(mutex,
		B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING, isWired);
	if ((oldValue & B_USER_MUTEX_LOCKED) == 0
			|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
		// possibly unset waiting flag
		if ((oldValue & B_USER_MUTEX_WAITING) == 0) {
			rw_lock_read_unlock(&entry->lock);
			rw_lock_write_lock(&entry->lock);
			if (entry->condition.EntriesCount() == 0)
				user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING, isWired);
			rw_lock_write_unlock(&entry->lock);
			rw_lock_read_lock(&entry->lock);
		}
		return true;
	}

	return false;
}


static status_t
user_mutex_lock_locked(UserMutexEntry* entry, int32* mutex,
	uint32 flags, bigtime_t timeout, ReadLocker& locker, bool isWired)
{
	if (user_mutex_prepare_to_lock(entry, mutex, isWired))
		return B_OK;

	status_t error = user_mutex_wait_locked(entry, flags, timeout, locker);

	// possibly unset waiting flag
	if (error != B_OK && entry->condition.EntriesCount() == 0) {
		WriteLocker writeLocker(entry->lock);
		if (entry->condition.EntriesCount() == 0)
			user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING, isWired);
	}

	return error;
}


static void
user_mutex_unblock(UserMutexEntry* entry, int32* mutex, uint32 flags, bool isWired)
{
	WriteLocker entryLocker(entry->lock);
	if (entry->condition.EntriesCount() == 0) {
		// Nobody is actually waiting at present.
		user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING, isWired);
		return;
	}

	int32 oldValue = 0;
	if ((flags & B_USER_MUTEX_UNBLOCK_ALL) == 0) {
		// This is not merely an unblock, but a hand-off.
		oldValue = user_atomic_or(mutex, B_USER_MUTEX_LOCKED, isWired);
		if ((oldValue & B_USER_MUTEX_LOCKED) != 0)
			return;
	}

	if ((flags & B_USER_MUTEX_UNBLOCK_ALL) != 0
			|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
		// unblock all waiting threads
		entry->condition.NotifyAll(B_OK);
	} else {
		if (!entry->condition.NotifyOne(B_OK))
			user_atomic_and(mutex, ~(int32)B_USER_MUTEX_LOCKED, isWired);
	}

	if (entry->condition.EntriesCount() == 0)
		user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING, isWired);
}


static status_t
user_mutex_sem_acquire_locked(UserMutexEntry* entry, int32* sem,
	uint32 flags, bigtime_t timeout, ReadLocker& locker, bool isWired)
{
	// The semaphore may have been released in the meantime, and we also
	// need to mark it as contended if it isn't already.
	int32 oldValue = user_atomic_get(sem, isWired);
	while (oldValue > -1) {
		int32 value = user_atomic_test_and_set(sem, oldValue - 1, oldValue, isWired);
		if (value == oldValue && value > 0)
			return B_OK;
		oldValue = value;
	}

	return user_mutex_wait_locked(entry, flags,
		timeout, locker);
}


static void
user_mutex_sem_release(UserMutexEntry* entry, int32* sem, bool isWired)
{
	WriteLocker entryLocker(entry->lock);
	if (entry->condition.NotifyOne(B_OK) == 0) {
		// no waiters - mark as uncontended and release
		int32 oldValue = user_atomic_get(sem, isWired);
		while (true) {
			int32 inc = oldValue < 0 ? 2 : 1;
			int32 value = user_atomic_test_and_set(sem, oldValue + inc, oldValue, isWired);
			if (value == oldValue)
				return;
			oldValue = value;
		}
	}

	if (entry->condition.EntriesCount() == 0) {
		// mark the semaphore uncontended
		user_atomic_test_and_set(sem, 0, -1, isWired);
	}
}


// #pragma mark - syscalls


struct UserMutexContextFetcher {
	UserMutexContextFetcher(int32* mutex, uint32 flags)
		:
		fInitStatus(B_OK),
		fShared((flags & B_USER_MUTEX_SHARED) != 0),
		fAddress(0)
	{
		if (!fShared) {
			fContext = get_team_user_mutex_context();
			if (fContext == NULL) {
				fInitStatus = B_NO_MEMORY;
				return;
			}

			fAddress = (addr_t)mutex;
		} else {
			fContext = &sSharedUserMutexContext;

			// wire the page and get the physical address
			fInitStatus = vm_wire_page(B_CURRENT_TEAM, (addr_t)mutex, true,
				&fWiringInfo);
			if (fInitStatus != B_OK)
				return;
			fAddress = fWiringInfo.physicalAddress;
		}
	}

	~UserMutexContextFetcher()
	{
		if (fInitStatus != B_OK)
			return;

		if (fShared)
			vm_unwire_page(&fWiringInfo);
	}

	status_t InitCheck() const
		{ return fInitStatus; }

	struct user_mutex_context* Context() const
		{ return fContext; }

	generic_addr_t Address() const
		{ return fAddress; }

	bool IsWired() const
		{ return fShared; }

private:
	status_t fInitStatus;
	bool fShared;
	struct user_mutex_context* fContext;
	VMPageWiringInfo fWiringInfo;
	generic_addr_t fAddress;
};


static status_t
user_mutex_lock(int32* mutex, const char* name, uint32 flags, bigtime_t timeout)
{
	UserMutexContextFetcher contextFetcher(mutex, flags);
	if (contextFetcher.InitCheck() != B_OK)
		return contextFetcher.InitCheck();

	// get the lock
	UserMutexEntry* entry = get_user_mutex_entry(contextFetcher.Context(),
		contextFetcher.Address());
	if (entry == NULL)
		return B_NO_MEMORY;
	status_t error = B_OK;
	{
		ReadLocker entryLocker(entry->lock);
		error = user_mutex_lock_locked(entry, mutex,
			flags, timeout, entryLocker, contextFetcher.IsWired());
	}
	put_user_mutex_entry(contextFetcher.Context(), entry);

	return error;
}


static status_t
user_mutex_switch_lock(int32* fromMutex, uint32 fromFlags,
	int32* toMutex, const char* name, uint32 toFlags, bigtime_t timeout)
{
	UserMutexContextFetcher fromFetcher(fromMutex, fromFlags);
	if (fromFetcher.InitCheck() != B_OK)
		return fromFetcher.InitCheck();

	UserMutexContextFetcher toFetcher(toMutex, toFlags);
	if (toFetcher.InitCheck() != B_OK)
		return toFetcher.InitCheck();

	// unlock the first mutex and lock the second one
	UserMutexEntry* fromEntry = NULL,
		*toEntry = get_user_mutex_entry(toFetcher.Context(), toFetcher.Address());
	if (toEntry == NULL)
		return B_NO_MEMORY;
	status_t error = B_OK;
	{
		ConditionVariableEntry waiter;

		bool alreadyLocked = false;
		{
			ReadLocker entryLocker(toEntry->lock);
			alreadyLocked = user_mutex_prepare_to_lock(toEntry, toMutex,
				toFetcher.IsWired());
			if (!alreadyLocked)
				toEntry->condition.Add(&waiter);
		}

		const int32 oldValue = user_atomic_and(fromMutex, ~(int32)B_USER_MUTEX_LOCKED,
			fromFetcher.IsWired());
		if ((oldValue & B_USER_MUTEX_WAITING) != 0) {
			fromEntry = get_user_mutex_entry(fromFetcher.Context(),
				fromFetcher.Address(), true);
			 if (fromEntry != NULL) {
				 user_mutex_unblock(fromEntry, fromMutex, fromFlags,
					 fromFetcher.IsWired());
			 }
		}

		if (!alreadyLocked)
			error = waiter.Wait(toFlags, timeout);
	}
	put_user_mutex_entry(fromFetcher.Context(), fromEntry);
	put_user_mutex_entry(toFetcher.Context(), toEntry);

	return error;
}


status_t
_user_mutex_lock(int32* mutex, const char* name, uint32 flags,
	bigtime_t timeout)
{
	if (mutex == NULL || !IS_USER_ADDRESS(mutex) || (addr_t)mutex % 4 != 0)
		return B_BAD_ADDRESS;

	syscall_restart_handle_timeout_pre(flags, timeout);

	status_t error = user_mutex_lock(mutex, name, flags | B_CAN_INTERRUPT,
		timeout);

	return syscall_restart_handle_timeout_post(error, timeout);
}


status_t
_user_mutex_unblock(int32* mutex, uint32 flags)
{
	if (mutex == NULL || !IS_USER_ADDRESS(mutex) || (addr_t)mutex % 4 != 0)
		return B_BAD_ADDRESS;

	UserMutexContextFetcher contextFetcher(mutex, flags);
	if (contextFetcher.InitCheck() != B_OK)
		return contextFetcher.InitCheck();
	struct user_mutex_context* context = contextFetcher.Context();

	// In the case where there is no entry, we must hold the read lock until we
	// unset WAITING, because otherwise some other thread could initiate a wait.
	ReadLocker tableReadLocker(context->lock);
	UserMutexEntry* entry = get_user_mutex_entry(context,
		contextFetcher.Address(), true, true);
	if (entry == NULL) {
		user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING, contextFetcher.IsWired());
		tableReadLocker.Unlock();
	} else {
		tableReadLocker.Unlock();
		user_mutex_unblock(entry, mutex, flags, contextFetcher.IsWired());
	}
	put_user_mutex_entry(context, entry);

	return B_OK;
}


status_t
_user_mutex_switch_lock(int32* fromMutex, uint32 fromFlags,
	int32* toMutex, const char* name, uint32 toFlags, bigtime_t timeout)
{
	if (fromMutex == NULL || !IS_USER_ADDRESS(fromMutex)
			|| (addr_t)fromMutex % 4 != 0 || toMutex == NULL
			|| !IS_USER_ADDRESS(toMutex) || (addr_t)toMutex % 4 != 0) {
		return B_BAD_ADDRESS;
	}

	return user_mutex_switch_lock(fromMutex, fromFlags, toMutex, name,
		toFlags | B_CAN_INTERRUPT, timeout);
}


status_t
_user_mutex_sem_acquire(int32* sem, const char* name, uint32 flags,
	bigtime_t timeout)
{
	if (sem == NULL || !IS_USER_ADDRESS(sem) || (addr_t)sem % 4 != 0)
		return B_BAD_ADDRESS;

	syscall_restart_handle_timeout_pre(flags, timeout);

	UserMutexContextFetcher contextFetcher(sem, flags);
	if (contextFetcher.InitCheck() != B_OK)
		return contextFetcher.InitCheck();
	struct user_mutex_context* context = contextFetcher.Context();

	UserMutexEntry* entry = get_user_mutex_entry(context, contextFetcher.Address());
	if (entry == NULL)
		return B_NO_MEMORY;
	status_t error;
	{
		ReadLocker entryLocker(entry->lock);
		error = user_mutex_sem_acquire_locked(entry, sem,
			flags | B_CAN_INTERRUPT, timeout, entryLocker, contextFetcher.IsWired());
	}
	put_user_mutex_entry(context, entry);

	return syscall_restart_handle_timeout_post(error, timeout);
}


status_t
_user_mutex_sem_release(int32* sem, uint32 flags)
{
	if (sem == NULL || !IS_USER_ADDRESS(sem) || (addr_t)sem % 4 != 0)
		return B_BAD_ADDRESS;

	UserMutexContextFetcher contextFetcher(sem, flags);
	if (contextFetcher.InitCheck() != B_OK)
		return contextFetcher.InitCheck();
	struct user_mutex_context* context = contextFetcher.Context();

	UserMutexEntry* entry = get_user_mutex_entry(context,
		contextFetcher.Address());
	{
		user_mutex_sem_release(entry, sem, contextFetcher.IsWired());
	}
	put_user_mutex_entry(context, entry);

	return B_OK;
}
