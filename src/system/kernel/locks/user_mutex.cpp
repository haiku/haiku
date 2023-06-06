/*
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


struct UserMutexEntry {
	phys_addr_t			address;
	UserMutexEntry*		hash_next;
	int32				ref_count;

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
static mutex sUserMutexTableLock = MUTEX_INITIALIZER("user mutex table");


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
get_user_mutex_entry(phys_addr_t address)
{
	UserMutexEntry* entry = sUserMutexTable.Lookup(address);
	if (entry != NULL) {
		atomic_add(&entry->ref_count, 1);
		return entry;
	}

	entry = new(std::nothrow) UserMutexEntry;
	if (entry == NULL)
		return entry;

	entry->address = address;
	entry->ref_count = 1;
	entry->condition.Init(entry, "UserMutexEntry");

	sUserMutexTable.Insert(entry);
	return entry;
}


static void
put_user_mutex_entry(UserMutexEntry* entry, MutexLocker& locker)
{
	const phys_addr_t address = entry->address;
	if (atomic_add(&entry->ref_count, -1) != 1)
		return;

	locker.Lock();

	// Was it removed & deleted while we were waiting for the lock?
	if (sUserMutexTable.Lookup(address) != entry)
		return;

	// Or did someone else acquire a reference to it?
	if (atomic_get(&entry->ref_count) > 0)
		return;

	sUserMutexTable.Remove(entry);
	delete entry;
}


static status_t
user_mutex_wait_locked(int32* mutex, phys_addr_t physicalAddress, const char* name,
	uint32 flags, bigtime_t timeout, MutexLocker& locker)
{
	// add or get the entry from the table
	UserMutexEntry* entry = get_user_mutex_entry(physicalAddress);
	if (entry == NULL)
		return B_NO_MEMORY;

	// wait
	ConditionVariableEntry waiter;
	entry->condition.Add(&waiter);
	locker.Unlock();

	status_t error = waiter.Wait(flags, timeout);

	// this will re-lock only if necessary
	put_user_mutex_entry(entry, locker);

	return error;
}


static bool
user_mutex_prepare_to_lock(int32* mutex)
{
	int32 oldValue = user_atomic_or(mutex,
		B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING);
	if ((oldValue & B_USER_MUTEX_LOCKED) == 0
			|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
		// clear the waiting flag and be done
		if ((oldValue & B_USER_MUTEX_WAITING) == 0)
			user_atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
		return true;
	}

	return false;
}


static status_t
user_mutex_lock_locked(int32* mutex, phys_addr_t physicalAddress,
	const char* name, uint32 flags, bigtime_t timeout, MutexLocker& locker)
{
	if (user_mutex_prepare_to_lock(mutex))
		return B_OK;

	return user_mutex_wait_locked(mutex, physicalAddress, name,
		flags, timeout, locker);
}


static void
user_mutex_unblock_locked(int32* mutex, phys_addr_t physicalAddress, uint32 flags)
{
	UserMutexEntry* entry = sUserMutexTable.Lookup(physicalAddress);
	if (entry == NULL) {
		// no one is waiting
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
user_mutex_sem_acquire_locked(int32* sem, phys_addr_t physicalAddress,
	const char* name, uint32 flags, bigtime_t timeout, MutexLocker& locker)
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

	return user_mutex_wait_locked(sem, physicalAddress, name, flags,
		timeout, locker);
}


static void
user_mutex_sem_release_locked(int32* sem, phys_addr_t physicalAddress)
{
	UserMutexEntry* entry = sUserMutexTable.Lookup(physicalAddress);
	if (!entry) {
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
	{
		MutexLocker locker(sUserMutexTableLock);
		error = user_mutex_lock_locked(mutex, wiringInfo.physicalAddress, name,
			flags, timeout, locker);
	}

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
	{
		MutexLocker locker(sUserMutexTableLock);

		const bool alreadyLocked = user_mutex_prepare_to_lock(toMutex);
		user_atomic_and(fromMutex, ~(int32)B_USER_MUTEX_LOCKED);
		user_mutex_unblock_locked(fromMutex, fromWiringInfo.physicalAddress,
			flags);

		if (!alreadyLocked) {
			error = user_mutex_lock_locked(toMutex, toWiringInfo.physicalAddress,
				name, flags, timeout, locker);
		}
	}

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

	{
		MutexLocker locker(sUserMutexTableLock);
		user_mutex_unblock_locked(mutex, wiringInfo.physicalAddress, flags);
	}

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

	{
		MutexLocker locker(sUserMutexTableLock);
		error = user_mutex_sem_acquire_locked(sem, wiringInfo.physicalAddress,
			name, flags | B_CAN_INTERRUPT, timeout, locker);
	}

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

	{
		MutexLocker locker(sUserMutexTableLock);
		user_mutex_sem_release_locked(sem, wiringInfo.physicalAddress);
	}

	vm_unwire_page(&wiringInfo);
	return B_OK;
}
