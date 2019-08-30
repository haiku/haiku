/*
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include <syscalls.h>


#ifdef ATOMIC_FUNCS_ARE_SYSCALLS

void
atomic_set(int32 *value, int32 newValue)
{
	_kern_atomic_set(value, newValue);
}


int32
atomic_get_and_set(int32 *value, int32 newValue)
{
	return _kern_atomic_get_and_set(value, newValue);
}


int32
atomic_test_and_set(int32 *value, int32 newValue, int32 testAgainst)
{
	return _kern_atomic_test_and_set(value, newValue, testAgainst);
}


int32
atomic_add(int32 *value, int32 addValue)
{
	return _kern_atomic_add(value, addValue);
}


int32
atomic_and(int32 *value, int32 andValue)
{
	return _kern_atomic_and(value, andValue);
}


int32
atomic_or(int32 *value, int32 orValue)
{
	return _kern_atomic_or(value, orValue);
}


int32
atomic_get(int32 *value)
{
	return _kern_atomic_get(value);
}


#endif	/* ATOMIC_FUNCS_ARE_SYSCALLS */

#ifdef ATOMIC64_FUNCS_ARE_SYSCALLS

void
atomic_set64(int64 *value, int64 newValue)
{
	_kern_atomic_set64(value, newValue);
}


int64
atomic_test_and_set64(int64 *value, int64 newValue, int64 testAgainst)
{
	return _kern_atomic_test_and_set64(value, newValue, testAgainst);
}


int64
atomic_add64(int64 *value, int64 addValue)
{
	return _kern_atomic_add64(value, addValue);
}


int64
atomic_and64(int64 *value, int64 andValue)
{
	return _kern_atomic_and64(value, andValue);
}


int64
atomic_or64(int64 *value, int64 orValue)
{
	return _kern_atomic_or64(value, orValue);
}


int64
atomic_get64(int64 *value)
{
	return _kern_atomic_get64(value);
}


#endif	/* ATOMIC64_FUNCS_ARE_SYSCALLS */

#if defined(__arm__) && !defined(__clang__)

/* GCC compatibility: libstdc++ needs this one.
 * TODO: Update libstdc++ and drop this.
 * cf. http://fedoraproject.org/wiki/Architectures/ARM/GCCBuiltInAtomicOperations
 */
extern int32_t __sync_fetch_and_add_4(int32_t *value, int32_t addValue);

extern int32_t __sync_fetch_and_add_4(int32_t *value, int32_t addValue)
{
	return atomic_add((int32 *)value, addValue);
}

#endif
