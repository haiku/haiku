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

#ifdef ATOMIC64_FUNCS_ARE_SYSCALLS

/*
 * NOTE: Unlike their 32-bit counterparts, these functions can use
 * spinlocks safely currently, as no atomic 64-bit operations are
 * done in the spinlock code. If this ever changes, this code will
 * have to change.
 *
 * This code is here for ARMv6, which cannot do proper 64-bit atomic
 * operations. Anything newer is capable, and does therefore not
 * depend on this code.
 */


static spinlock atomic_lock = B_SPINLOCK_INITIALIZER;


void
atomic_set64(int64 *value, int64 newValue)
{
	SpinLocker locker(&atomic_lock);

	*value = newValue;
}


int64
atomic_get_and_set64(int64 *value, int64 newValue)
{
	SpinLocker locker(&atomic_lock);

	int64 oldValue = *value;
	*value = newValue;
	return oldValue;
}


int64
atomic_test_and_set64(int64 *value, int64 newValue, int64 testAgainst)
{
	SpinLocker locker(&atomic_lock);

	int64 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	return oldValue;
}


int64
atomic_add64(int64 *value, int64 addValue)
{
	SpinLocker locker(&atomic_lock);

	int64 oldValue = *value;
	*value += addValue;
	return oldValue;
}


int64
atomic_and64(int64 *value, int64 andValue)
{
	SpinLocker locker(&atomic_lock);

	int64 oldValue = *value;
	*value &= andValue;
	return oldValue;
}


int64
atomic_or64(int64 *value, int64 orValue)
{
	SpinLocker locker(&atomic_lock);

	int64 oldValue = *value;
	*value |= orValue;
	return oldValue;
}


int64
atomic_get64(int64 *value)
{
	SpinLocker locker(&atomic_lock);
	return *value;
}


int64
_user_atomic_get_and_set64(int64 *value, int64 newValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		int64 oldValue = atomic_get_and_set64(value, newValue);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}


void
_user_atomic_set64(int64 *value, int64 newValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		atomic_set64(value, newValue);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return;
	}

access_violation:
	// XXX kill application
	return;
}


int64
_user_atomic_test_and_set64(int64 *value, int64 newValue, int64 testAgainst)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		int64 oldValue = atomic_test_and_set64(value, newValue, testAgainst);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}


int64
_user_atomic_add64(int64 *value, int64 addValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		int64 oldValue = atomic_add64(value, addValue);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}


int64
_user_atomic_and64(int64 *value, int64 andValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		int64 oldValue = atomic_and64(value, andValue);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}


int64
_user_atomic_or64(int64 *value, int64 orValue)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		int64 oldValue = atomic_or64(value, orValue);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}


int64
_user_atomic_get64(int64 *value)
{
	if (IS_USER_ADDRESS(value)
		&& lock_memory((void *)value, sizeof(int64), B_READ_DEVICE) == B_OK) {
		int64 oldValue = atomic_get64(value);
		unlock_memory((void *)value, sizeof(int64), B_READ_DEVICE);
		return oldValue;
	}

access_violation:
	// XXX kill application
	return -1;
}

#endif /* ATOMIC64_FUNCS_ARE_SYSCALLS */
