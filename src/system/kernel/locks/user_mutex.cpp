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
#include <util/OpenHashTable.h>
#include <vm/vm.h>
#include <vm/VMArea.h>


/*! One UserMutexEntry corresponds to one mutex address.
 *
 * The mutex's "waiting" state is controlled by the rw_lock: a waiter acquires
 * a "read" lock before initiating a wait, and an unblocker acquires a "write"
 * lock. That way, unblockers can be sure that no waiters will start waiting
 * during unblock, and they can thus safely (without races) unset WAITING.
 */
struct UserMutexEntry {
	phys_addr_t			address;
	UserMutexEntry*		hash_next;
	int32				ref_count;

	rw_lock				lock;
	ConditionVariable	condition;
};

struct UserMutexHashDefinition {
	typedef phys_addr_t		KeyType;
	typedef UserMutexEntry	ValueType;

	size_t HashKey(phys_addr_t key) const
	{
		return key >> 2;
	}

	size_t Hash(const UserMutexEntry* value) const
	{
		return HashKey(value->address);
	}

	bool Compare(phys_addr_t key, const UserMutexEntry* value) const
	{
		return value->address == key;
	}

	UserMutexEntry*& GetLink(UserMutexEntry* value) const
	{
		return value->hash_next;
	}
};

typedef BOpenHashTable<UserMutexHashDefinition> UserMutexTable;


static UserMutexTable sUserMutexTable;
static rw_lock sUserMutexTableLock = RW_LOCK_INITIALIZER("user mutex table");


// #pragma mark - user atomics


static int32
user_atomic_or(int32* value, int32 orValue)
{
	set_ac();
	int32 result = atomic_or(value, orValue);
	clear_ac();
	return result;
}


static int32
user_atomic_and(int32* value, int32 orValue)
{
	set_ac();
	int32 result = atomic_and(value, orValue);
	clear_ac();
	return result;
}


static int32
user_atomic_get(int32* value)
{
	set_ac();
	int32 result = atomic_get(value);
	clear_ac();
	return result;
}


static int32
user_atomic_test_and_set(int32* value, int32 newValue, int32 testAgainst)
{
	set_ac();
	int32 result = atomic_test_and_set(value, newValue, testAgainst);
	clear_ac();
	return result;
}


// #pragma mark - user mutex entries


static UserMutexEntry*
get_user_mutex_entry(phys_addr_t address, bool noInsert = false, bool alreadyLocked = false)
{
	ReadLocker tableReadLocker;
	if (!alreadyLocked)
		tableReadLocker.SetTo(sUserMutexTableLock, false);

	UserMutexEntry* entry = sUserMutexTable.Lookup(address);
	if (entry != NULL) {
		atomic_add(&entry->ref_count, 1);
		return entry;
	} else if (noInsert)
		return entry;

	tableReadLocker.Unlock();
	WriteLocker tableWriteLocker(sUserMutexTableLock);

	entry = sUserMutexTable.Lookup(address);
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
	entry->condition.Init(entry, "UserMutexEntry");

	sUserMutexTable.Insert(entry);
	return entry;
}


static void
put_user_mutex_entry(UserMutexEntry* entry)
{
	if (entry == NULL)
		return;

	const phys_addr_t address = entry->address;
	if (atomic_add(&entry->ref_count, -1) != 1)
		return;

	WriteLocker tableWriteLocker(sUserMutexTableLock);

	// Was it removed & deleted while we were waiting for the lock?
	if (sUserMutexTable.Lookup(address) != entry)
		return;

	// Or did someone else acquire a reference to it?
	if (atomic_get(&entry->ref_count) > 0)
		return;

	sUserMutexTable.Remove(entry);
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
user_mutex_prepare_to_lock(UserMutexEntry* entry, int32* mutex)
{
	ASSERT_READ_LOCKED_RW_LOCK(&entry->lock);

	int32 oldValue = user_atomic_or(mutex,
		B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING);
	if ((oldValue & B_USER_MUTEX_LOCKED) == 0
			|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
		// possibly unset waiting flag
		if ((oldValue & B_USER_MUTEX_WAITING) == 0) {
			rw_lock_read_unlock(&entry->lock);
			rw_lock_write_lock(&entry->lock);
			if (entry->condition.EntriesCount() == 0)
				user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
			rw_lock_write_unlock(&entry->lock);
			rw_lock_read_lock(&entry->lock);
		}
		return true;
	}

	return false;
}


static status_t
user_mutex_lock_locked(UserMutexEntry* entry, int32* mutex,
	uint32 flags, bigtime_t timeout, ReadLocker& locker)
{
	if (user_mutex_prepare_to_lock(entry, mutex))
		return B_OK;

	status_t error = user_mutex_wait_locked(entry, flags, timeout, locker);

	// possibly unset waiting flag
	if (error != B_OK && entry->condition.EntriesCount() == 0) {
		WriteLocker writeLocker(entry->lock);
		if (entry->condition.EntriesCount() == 0)
			user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
	}

	return error;
}


static void
user_mutex_unblock(UserMutexEntry* entry, int32* mutex, uint32 flags)
{
	WriteLocker entryLocker(entry->lock);
	if (entry->condition.EntriesCount() == 0) {
		// Nobody is actually waiting at present.
		user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
		return;
	}

	int32 oldValue = 0;
	if ((flags & B_USER_MUTEX_UNBLOCK_ALL) == 0) {
		// This is not merely an unblock, but a hand-off.
		oldValue = user_atomic_or(mutex, B_USER_MUTEX_LOCKED);
		if ((oldValue & B_USER_MUTEX_LOCKED) != 0)
			return;
	}

	if ((flags & B_USER_MUTEX_UNBLOCK_ALL) != 0
			|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
		// unblock and dequeue all the waiting threads
		entry->condition.NotifyAll(B_OK);
	} else {
		entry->condition.NotifyOne(B_OK);
	}

	if (entry->condition.EntriesCount() == 0)
		user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
}


static status_t
user_mutex_sem_acquire_locked(UserMutexEntry* entry, int32* sem,
	uint32 flags, bigtime_t timeout, ReadLocker& locker)
{
	// The semaphore may have been released in the meantime, and we also
	// need to mark it as contended if it isn't already.
	int32 oldValue = user_atomic_get(sem);
	while (oldValue > -1) {
		int32 value = user_atomic_test_and_set(sem, oldValue - 1, oldValue);
		if (value == oldValue && value > 0)
			return B_OK;
		oldValue = value;
	}

	return user_mutex_wait_locked(entry, flags,
		timeout, locker);
}


static void
user_mutex_sem_release(UserMutexEntry* entry, int32* sem)
{
	if (entry == NULL) {
		// no waiters - mark as uncontended and release
		int32 oldValue = user_atomic_get(sem);
		while (true) {
			int32 inc = oldValue < 0 ? 2 : 1;
			int32 value = user_atomic_test_and_set(sem, oldValue + inc, oldValue);
			if (value == oldValue)
				return;
			oldValue = value;
		}
	}

	WriteLocker entryLocker(entry->lock);
	entry->condition.NotifyOne(B_OK);
	if (entry->condition.EntriesCount() == 0) {
		// mark the semaphore uncontended
		user_atomic_test_and_set(sem, 0, -1);
	}
}


static status_t
user_mutex_lock(int32* mutex, const char* name, uint32 flags, bigtime_t timeout)
{
	// wire the page and get the physical address
	VMPageWiringInfo wiringInfo;
	status_t error = vm_wire_page(B_CURRENT_TEAM, (addr_t)mutex, true,
		&wiringInfo);
	if (error != B_OK)
		return error;

	// get the lock
	UserMutexEntry* entry = get_user_mutex_entry(wiringInfo.physicalAddress);
	if (entry == NULL)
		return B_NO_MEMORY;
	{
		ReadLocker entryLocker(entry->lock);
		error = user_mutex_lock_locked(entry, mutex,
			flags, timeout, entryLocker);
	}
	put_user_mutex_entry(entry);

	// unwire the page
	vm_unwire_page(&wiringInfo);

	return error;
}


static status_t
user_mutex_switch_lock(int32* fromMutex, int32* toMutex, const char* name,
	uint32 flags, bigtime_t timeout)
{
	// wire the pages and get the physical addresses
	VMPageWiringInfo fromWiringInfo;
	status_t error = vm_wire_page(B_CURRENT_TEAM, (addr_t)fromMutex, true,
		&fromWiringInfo);
	if (error != B_OK)
		return error;

	VMPageWiringInfo toWiringInfo;
	error = vm_wire_page(B_CURRENT_TEAM, (addr_t)toMutex, true, &toWiringInfo);
	if (error != B_OK) {
		vm_unwire_page(&fromWiringInfo);
		return error;
	}

	// unlock the first mutex and lock the second one
	UserMutexEntry* fromEntry = NULL,
		*toEntry = get_user_mutex_entry(toWiringInfo.physicalAddress);
	if (toEntry == NULL)
		return B_NO_MEMORY;
	{
		ConditionVariableEntry waiter;

		bool alreadyLocked = false;
		{
			ReadLocker entryLocker(toEntry->lock);
			alreadyLocked = user_mutex_prepare_to_lock(toEntry, toMutex);
			if (!alreadyLocked)
				toEntry->condition.Add(&waiter);
		}

		const int32 oldValue = user_atomic_and(fromMutex, ~(int32)B_USER_MUTEX_LOCKED);
		if ((oldValue & B_USER_MUTEX_WAITING) != 0)
			 fromEntry = get_user_mutex_entry(fromWiringInfo.physicalAddress, true);
		if (fromEntry != NULL)
			user_mutex_unblock(fromEntry, fromMutex, flags);

		if (!alreadyLocked)
			error = waiter.Wait(flags, timeout);
	}
	put_user_mutex_entry(fromEntry);
	put_user_mutex_entry(toEntry);

	// unwire the pages
	vm_unwire_page(&toWiringInfo);
	vm_unwire_page(&fromWiringInfo);

	return error;
}


// #pragma mark - kernel private


void
user_mutex_init()
{
	if (sUserMutexTable.Init() != B_OK)
		panic("user_mutex_init(): Failed to init table!");
}


// #pragma mark - syscalls


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

	// wire the page and get the physical address
	VMPageWiringInfo wiringInfo;
	status_t error = vm_wire_page(B_CURRENT_TEAM, (addr_t)mutex, true,
		&wiringInfo);
	if (error != B_OK)
		return error;

	// In the case where there is no entry, we must hold the read lock until we
	// unset WAITING, because otherwise some other thread could initiate a wait.
	ReadLocker tableReadLocker(sUserMutexTableLock);
	UserMutexEntry* entry = get_user_mutex_entry(wiringInfo.physicalAddress, true, true);
	if (entry == NULL) {
		user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
		tableReadLocker.Unlock();
	} else {
		tableReadLocker.Unlock();
		user_mutex_unblock(entry, mutex, flags);
	}
	put_user_mutex_entry(entry);

	vm_unwire_page(&wiringInfo);

	return B_OK;
}


status_t
_user_mutex_switch_lock(int32* fromMutex, int32* toMutex, const char* name,
	uint32 flags, bigtime_t timeout)
{
	if (fromMutex == NULL || !IS_USER_ADDRESS(fromMutex)
			|| (addr_t)fromMutex % 4 != 0 || toMutex == NULL
			|| !IS_USER_ADDRESS(toMutex) || (addr_t)toMutex % 4 != 0) {
		return B_BAD_ADDRESS;
	}

	return user_mutex_switch_lock(fromMutex, toMutex, name,
		flags | B_CAN_INTERRUPT, timeout);
}


status_t
_user_mutex_sem_acquire(int32* sem, const char* name, uint32 flags,
	bigtime_t timeout)
{
	if (sem == NULL || !IS_USER_ADDRESS(sem) || (addr_t)sem % 4 != 0)
		return B_BAD_ADDRESS;

	syscall_restart_handle_timeout_pre(flags, timeout);

	// wire the page and get the physical address
	VMPageWiringInfo wiringInfo;
	status_t error = vm_wire_page(B_CURRENT_TEAM, (addr_t)sem, true,
		&wiringInfo);
	if (error != B_OK)
		return error;

	UserMutexEntry* entry = get_user_mutex_entry(wiringInfo.physicalAddress);
	if (entry == NULL)
		return B_NO_MEMORY;
	{
		ReadLocker entryLocker(entry->lock);
		error = user_mutex_sem_acquire_locked(entry, sem,
			flags | B_CAN_INTERRUPT, timeout, entryLocker);
	}
	put_user_mutex_entry(entry);

	vm_unwire_page(&wiringInfo);
	return syscall_restart_handle_timeout_post(error, timeout);
}


status_t
_user_mutex_sem_release(int32* sem)
{
	if (sem == NULL || !IS_USER_ADDRESS(sem) || (addr_t)sem % 4 != 0)
		return B_BAD_ADDRESS;

	// wire the page and get the physical address
	VMPageWiringInfo wiringInfo;
	status_t error = vm_wire_page(B_CURRENT_TEAM, (addr_t)sem, true,
		&wiringInfo);
	if (error != B_OK)
		return error;

	UserMutexEntry* entry = get_user_mutex_entry(wiringInfo.physicalAddress, true);
	{
		user_mutex_sem_release(entry, sem);
	}
	put_user_mutex_entry(entry);

	vm_unwire_page(&wiringInfo);
	return B_OK;
}
