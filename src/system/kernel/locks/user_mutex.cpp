/*
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


struct UserMutexEntry;
typedef DoublyLinkedList<UserMutexEntry> UserMutexEntryList;

struct UserMutexEntry : public DoublyLinkedListLinkImpl<UserMutexEntry> {
	addr_t				address;
	ConditionVariable	condition;
	bool				locked;
	UserMutexEntryList	otherEntries;
	UserMutexEntry*		hashNext;
};

struct UserMutexHashDefinition {
	typedef addr_t			KeyType;
	typedef UserMutexEntry	ValueType;

	size_t HashKey(addr_t key) const
	{
		return key >> 2;
	}

	size_t Hash(const UserMutexEntry* value) const
	{
		return HashKey(value->address);
	}

	bool Compare(addr_t key, const UserMutexEntry* value) const
	{
		return value->address == key;
	}

	UserMutexEntry*& GetLink(UserMutexEntry* value) const
	{
		return value->hashNext;
	}
};

typedef BOpenHashTable<UserMutexHashDefinition> UserMutexTable;


static UserMutexTable sUserMutexTable;
static mutex sUserMutexTableLock = MUTEX_INITIALIZER("user mutex table");


static void
add_user_mutex_entry(UserMutexEntry* entry)
{
	UserMutexEntry* firstEntry = sUserMutexTable.Lookup(entry->address);
	if (firstEntry != NULL)
		firstEntry->otherEntries.Add(entry);
	else
		sUserMutexTable.Insert(entry);
}


static bool
remove_user_mutex_entry(UserMutexEntry* entry)
{
	UserMutexEntry* firstEntry = sUserMutexTable.Lookup(entry->address);
	if (firstEntry != entry) {
		// The entry is not the first entry in the table. Just remove it from
		// the first entry's list.
		firstEntry->otherEntries.Remove(entry);
		return true;
	}

	// The entry is the first entry in the table. Remove it from the table and,
	// if any, add the next entry to the table.
	sUserMutexTable.Remove(entry);

	firstEntry = entry->otherEntries.RemoveHead();
	if (firstEntry != NULL) {
		firstEntry->otherEntries.MoveFrom(&entry->otherEntries);
		sUserMutexTable.Insert(firstEntry);
		return true;
	}

	return false;
}


static status_t
user_mutex_lock_locked(vint32* mutex, addr_t physicalAddress, const char* name,
	uint32 flags, bigtime_t timeout, MutexLocker& locker)
{
	// mark the mutex locked + waiting
	int32 oldValue = atomic_or(mutex,
		B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING);

	// The mutex might have been unlocked (or disabled) in the meantime.
	if ((oldValue & (B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING)) == 0
			|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
		// clear the waiting flag and be done
		atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
		return B_OK;
	}

	// we have to wait

	// add the entry to the table
	UserMutexEntry entry;
	entry.address = physicalAddress;
	entry.locked = false;
	add_user_mutex_entry(&entry);

	// wait
	ConditionVariableEntry waitEntry;
	entry.condition.Init((void*)physicalAddress, "user mutex");
	entry.condition.Add(&waitEntry);

	locker.Unlock();
	status_t error = waitEntry.Wait(flags, timeout);
	locker.Lock();

	// dequeue
	if (!remove_user_mutex_entry(&entry)) {
		// no one is waiting anymore -- clear the waiting flag
		atomic_and(mutex, ~(int32)B_USER_MUTEX_WAITING);
	}

	if (error != B_OK
			&& (entry.locked || (*mutex & B_USER_MUTEX_DISABLED) != 0)) {
		// timeout or interrupt, but the mutex was unlocked or disabled in time
		error = B_OK;
	}

	return error;
}


static void
user_mutex_unlock_locked(vint32* mutex, addr_t physicalAddress, uint32 flags)
{
	if (UserMutexEntry* entry = sUserMutexTable.Lookup(physicalAddress)) {
		// Someone is waiting -- set the locked flag. It might still be set,
		// but when using userland atomic operations, the caller will usually
		// have cleared it already.
		int32 oldValue = atomic_or(mutex, B_USER_MUTEX_LOCKED);

		// unblock the first thread
		entry->locked = true;
		entry->condition.NotifyOne();

		if ((flags & B_USER_MUTEX_UNBLOCK_ALL) != 0
				|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
			// unblock all the other waiting threads as well
			for (UserMutexEntryList::Iterator it
					= entry->otherEntries.GetIterator();
				UserMutexEntry* otherEntry = it.Next();) {
				otherEntry->locked = true;
				otherEntry->condition.NotifyOne();
			}
		}
	} else {
		// no one is waiting -- clear locked flag
		atomic_and(mutex, ~(int32)B_USER_MUTEX_LOCKED);
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
		user_mutex_unlock_locked(fromMutex, fromWiringInfo.physicalAddress,
			flags);

		error = user_mutex_lock_locked(toMutex, toWiringInfo.physicalAddress,
			name, flags, timeout, locker);
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
_user_mutex_unlock(int32* mutex, uint32 flags)
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
		user_mutex_unlock_locked(mutex, wiringInfo.physicalAddress, flags);
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
