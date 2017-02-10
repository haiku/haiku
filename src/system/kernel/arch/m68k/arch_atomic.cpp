/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>

#include <kernel.h>
#include <user_atomic.h>

/*
 * Emulation of 64 bit atomic functions.
 * Slow, using spinlocks...
 */

#warning M68K: detect 060 here
#if 0

static spinlock atomic_lock = 0;

int64
atomic_set64(vint64 *value, int64 newValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value = newValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_add64(vint64 *value, int64 addValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value += addValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_and64(vint64 *value, int64 andValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value &= andValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_or64(vint64 *value, int64 orValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value |= orValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_get64(vint64 *value)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
_user_atomic_set64(vint64 *value, int64 newValue)
{
	cpu_status status;
	int64 oldValue;
	if (!IS_USER_ADDRESS(value)
		|| lock_memory((void *)value, 8, B_READ_DEVICE) != B_OK)
		goto access_violation;

	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value = newValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	unlock_memory((void *)value, 8, B_READ_DEVICE);
	return oldValue;

access_violation:
	// XXX kill application
	return -1;
}

int64
_user_atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst)
{
	cpu_status status;
	int64 oldValue;
	if (!IS_USER_ADDRESS(value)
		|| lock_memory((void *)value, 8, B_READ_DEVICE) != B_OK)
		goto access_violation;

	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	unlock_memory((void *)value, 8, B_READ_DEVICE);
	return oldValue;

access_violation:
	// XXX kill application
	return -1;
}

int64
_user_atomic_add64(vint64 *value, int64 addValue)
{
	cpu_status status;
	int64 oldValue;
	if (!IS_USER_ADDRESS(value)
		|| lock_memory((void *)value, 8, B_READ_DEVICE) != B_OK)
		goto access_violation;

	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value += addValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	unlock_memory((void *)value, 8, B_READ_DEVICE);
	return oldValue;

access_violation:
	// XXX kill application
	return -1;
}

int64
_user_atomic_and64(vint64 *value, int64 andValue)
{
	cpu_status status;
	int64 oldValue;
	if (!IS_USER_ADDRESS(value)
		|| lock_memory((void *)value, 8, B_READ_DEVICE) != B_OK)
		goto access_violation;

	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value &= andValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	unlock_memory((void *)value, 8, B_READ_DEVICE);
	return oldValue;

access_violation:
	// XXX kill application
	return -1;
}

int64
_user_atomic_or64(vint64 *value, int64 orValue)
{
	cpu_status status;
	int64 oldValue;
	if (!IS_USER_ADDRESS(value)
		|| lock_memory((void *)value, 8, B_READ_DEVICE) != B_OK)
		goto access_violation;

	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	*value |= orValue;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	unlock_memory((void *)value, 8, B_READ_DEVICE);
	return oldValue;
access_violation:
	// XXX kill application
	return -1;
}

int64
_user_atomic_get64(vint64 *value)
{
	cpu_status status;
	int64 oldValue;
	if (!IS_USER_ADDRESS(value)
		|| lock_memory((void *)value, 8, B_READ_DEVICE) != B_OK)
		goto access_violation;

	status = disable_interrupts();
	acquire_spinlock(&atomic_lock);
	oldValue = *value;
	release_spinlock(&atomic_lock);
	restore_interrupts(status);
	unlock_memory((void *)value, 8, B_READ_DEVICE);
	return oldValue;

access_violation:
	// XXX kill application
	return -1;
}
#endif
