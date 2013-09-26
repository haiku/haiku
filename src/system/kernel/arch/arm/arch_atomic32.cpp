/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema, ithamar@upgrade-android.com
 */

#include <KernelExport.h>

#include <kernel.h>
#include <user_atomic.h>

#include <util/AutoLock.h>

#ifdef ATOMIC_FUNCS_ARE_SYSCALLS

/*
 * NOTE: These functions are _intentionally_ not using spinlocks, unlike
 * the 64 bit versions. The reason for this is that they are used by the
 * spinlock code itself, and therefore would deadlock.
 *
 * Since these are only really needed for ARMv5, which is not SMP anyway,
 * this is an acceptable compromise.
 */

int32
atomic_set(vint32 *value, int32 newValue)
{
	InterruptsLocker locker;

	int32 oldValue = *value;
	*value = newValue;
	return oldValue;
}

int32
atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst)
{
	InterruptsLocker locker;

	int32 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	return oldValue;
}

int32
atomic_add(vint32 *value, int32 addValue)
{
	InterruptsLocker locker;

	int32 oldValue = *value;
	*value += addValue;
	return oldValue;
}

int32
atomic_and(vint32 *value, int32 andValue)
{
	InterruptsLocker locker;

	int32 oldValue = *value;
	*value &= andValue;
	return oldValue;
}

int32
atomic_or(vint32 *value, int32 orValue)
{
	InterruptsLocker locker;

	int32 oldValue = *value;
	*value |= orValue;
	return oldValue;
}

int32
atomic_get(vint32 *value)
{
	InterruptsLocker locker;

	int32 oldValue = *value;
	return oldValue;
}

int32
_user_atomic_set(vint32 *value, int32 newValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int32), B_READ_DEVICE) == B_OK) {
		int32 oldValue = atomic_set(value, newValue);
		unlock_memory((void *)value, sizeof(int32), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

int32
_user_atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int32), B_READ_DEVICE) == B_OK) {
		int32 oldValue = atomic_test_and_set(value, newValue, testAgainst);
		unlock_memory((void *)value, sizeof(int32), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

int32
_user_atomic_add(vint32 *value, int32 addValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int32), B_READ_DEVICE) == B_OK) {
		int32 oldValue = atomic_add(value, addValue);
		unlock_memory((void *)value, sizeof(int32), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

int32
_user_atomic_and(vint32 *value, int32 andValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int32), B_READ_DEVICE) == B_OK) {
		int32 oldValue = atomic_and(value, andValue);
		unlock_memory((void *)value, sizeof(int32), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

int32
_user_atomic_or(vint32 *value, int32 orValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int32), B_READ_DEVICE) == B_OK) {
		int32 oldValue = atomic_or(value, orValue);
		unlock_memory((void *)value, sizeof(int32), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

int32
_user_atomic_get(vint32 *value)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int32), B_READ_DEVICE) == B_OK) {
		int32 oldValue = atomic_get(value);
		unlock_memory((void *)value, sizeof(int32), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

#endif /* ATOMIC_FUNCS_ARE_SYSCALLS */
