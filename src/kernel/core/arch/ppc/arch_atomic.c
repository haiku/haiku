/*
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */

#include <KernelExport.h>
#include <user_atomic.h>

/*
 * Emulation of 64 bit atomic functions.
 * Slow, using spinlocks...
 */

static spinlock kernel_lock = 0;
static spinlock user_lock = 0;

int64
atomic_set64(vint64 *value, int64 newValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&kernel_lock);
	oldValue = *value;
	*value = newValue;
	release_spinlock(&kernel_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&kernel_lock);
	oldValue = *value;
	if (*value == testAgainst)
		*value = newValue;
	release_spinlock(&kernel_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_add64(vint64 *value, int64 addValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&kernel_lock);
	oldValue = *value;
	*value += addValue;
	release_spinlock(&kernel_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_and64(vint64 *value, int64 andValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&kernel_lock);
	oldValue = *value;
	*value &= andValue;
	release_spinlock(&kernel_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_or64(vint64 *value, int64 orValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&kernel_lock);
	oldValue = *value;
	*value |= orValue;
	release_spinlock(&kernel_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
atomic_read64(vint64 *value)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&kernel_lock);
	oldValue = *value;
	release_spinlock(&kernel_lock);
	restore_interrupts(status);
	return oldValue;
}

int64
user_atomic_set64(vint64 *value, int64 newValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&user_lock);
	if ((addr)value < KERNEL_BASE || (addr)value > (KERNEL_TOP - 8))
		goto error;
	if (user_memcpy(&oldValue, value, 8) < 0)
		goto error;
	if (user_memcpy(value, &newValue, 8) < 0)
		goto error;
	release_spinlock(&user_lock);
	restore_interrupts(status);
	return oldValue;

error:
	release_spinlock(&user_lock);
	restore_interrupts(status);
	// XXX kill application
	return -1;
}

int64
user_atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&user_lock);
	if ((addr)value < KERNEL_BASE || (addr)value > (KERNEL_TOP - 8))
		goto error;
	if (user_memcpy(&oldValue, value, 8) < 0)
		goto error;
	if (oldValue == testAgainst)
		if (user_memcpy(value, &newValue, 8) < 0)
			goto error;
	release_spinlock(&user_lock);
	restore_interrupts(status);
	return oldValue;

error:
	release_spinlock(&user_lock);
	restore_interrupts(status);
	// XXX kill application
	return -1;
}

int64
user_atomic_add64(vint64 *value, int64 addValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&user_lock);
	if ((addr)value < KERNEL_BASE || (addr)value > (KERNEL_TOP - 8))
		goto error;
	if (user_memcpy(&oldValue, value, 8) < 0)
		goto error;
	addValue += oldValue;
	if (user_memcpy(value, &addValue, 8) < 0)
		goto error;
	release_spinlock(&user_lock);
	restore_interrupts(status);
	return oldValue;

error:
	release_spinlock(&user_lock);
	restore_interrupts(status);
	// XXX kill application
	return -1;
}

int64
user_atomic_and64(vint64 *value, int64 andValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&user_lock);
	if ((addr)value < KERNEL_BASE || (addr)value > (KERNEL_TOP - 8))
		goto error;
	if (user_memcpy(&oldValue, value, 8) < 0)
		goto error;
	andValue &= oldValue;
	if (user_memcpy(value, &andValue, 8) < 0)
		goto error;
	release_spinlock(&user_lock);
	restore_interrupts(status);
	return oldValue;

error:
	release_spinlock(&user_lock);
	restore_interrupts(status);
	// XXX kill application
	return -1;
}

int64
user_atomic_or64(vint64 *value, int64 orValue)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&user_lock);
	if ((addr)value < KERNEL_BASE || (addr)value > (KERNEL_TOP - 8))
		goto error;
	if (user_memcpy(&oldValue, value, 8) < 0)
		goto error;
	orValue |= oldValue;
	if (user_memcpy(value, &orValue, 8) < 0)
		goto error;
	release_spinlock(&user_lock);
	restore_interrupts(status);
	return oldValue;

error:
	release_spinlock(&user_lock);
	restore_interrupts(status);
	// XXX kill application
	return -1;
}

int64
user_atomic_read64(vint64 *value)
{
	cpu_status status;
	int64 oldValue;
	status = disable_interrupts();
	acquire_spinlock(&user_lock);
	if ((addr)value < KERNEL_BASE || (addr)value > (KERNEL_TOP - 8))
		goto error;
	if (user_memcpy(&oldValue, value, 8) < 0)
		goto error;
	release_spinlock(&user_lock);
	restore_interrupts(status);
	return oldValue;

error:
	release_spinlock(&user_lock);
	restore_interrupts(status);
	// XXX kill application
	return -1;
}
