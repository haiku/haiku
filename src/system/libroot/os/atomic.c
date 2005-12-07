/* 
** Copyright 2003, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <SupportDefs.h>

#include <syscalls.h>


#ifdef ATOMIC_FUNCS_ARE_SYSCALLS

int32
atomic_set(vint32 *value, int32 newValue)
{
	return _kern_atomic_set(value, newValue);
}

int32
atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst)
{
	return _kern_atomic_test_and_set(value, newValue, testAgainst);
}

int32
atomic_add(vint32 *value, int32 addValue)
{
	return _kern_atomic_add(value, addValue);
}

int32
atomic_and(vint32 *value, int32 andValue)
{
	return _kern_atomic_and(value, andValue);
}

int32
atomic_or(vint32 *value, int32 orValue)
{
	return _kern_atomic_or(value, orValue);
}

int32
atomic_get(vint32 *value)
{
	return _kern_atomic_get(value);
}

#endif	/* ATOMIC_FUNCS_ARE_SYSCALLS */

#ifdef ATOMIC64_FUNCS_ARE_SYSCALLS

int64
atomic_set64(vint64 *value, int64 newValue)
{
	return _kern_atomic_set64(value, newValue);
}

int64
atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst)
{
	return _kern_atomic_test_and_set64(value, newValue, testAgainst);
}

int64
atomic_add64(vint64 *value, int64 addValue)
{
	return _kern_atomic_add64(value, addValue);
}

int64
atomic_and64(vint64 *value, int64 andValue)
{
	return _kern_atomic_and64(value, andValue);
}

int64
atomic_or64(vint64 *value, int64 orValue)
{
	return _kern_atomic_or64(value, orValue);
}

int64
atomic_get64(vint64 *value)
{
	return _kern_atomic_get64(value);
}

#endif	/* ATOMIC64_FUNCS_ARE_SYSCALLS */
