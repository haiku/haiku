/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include <OS.h>

#include "fssh_atomic.h"


int32_t
fssh_atomic_set(vint32_t *value, int32_t newValue)
{
	return atomic_set((vint32*)value, newValue);
}


int32_t
fssh_atomic_test_and_set(vint32_t *value, int32_t newValue, int32_t testAgainst)
{
	return atomic_test_and_set((vint32*)value, newValue, testAgainst);
}


int32_t
fssh_atomic_add(vint32_t *value, int32_t addValue)
{
	return atomic_add((vint32*)value, addValue);
}


int32_t
fssh_atomic_and(vint32_t *value, int32_t andValue)
{
	return atomic_and((vint32*)value, andValue);
}


int32_t
fssh_atomic_or(vint32_t *value, int32_t orValue)	
{
	return atomic_or((vint32*)value, orValue);
}


int32_t
fssh_atomic_get(vint32_t *value)
{
	return atomic_get((vint32*)value);
}


int64_t
fssh_atomic_set64(vint64_t *value, int64_t newValue)
{
	return atomic_set64((vint64*)value, newValue);
}


int64_t
fssh_atomic_test_and_set64(vint64_t *value, int64_t newValue, int64_t testAgainst)
{
	return atomic_test_and_set64((vint64 *)value, newValue, testAgainst);
}


int64_t
fssh_atomic_add64(vint64_t *value, int64_t addValue)
{
	return atomic_add64((vint64*)value, addValue);
}


int64_t
fssh_atomic_and64(vint64_t *value, int64_t andValue)
{
	return atomic_and64((vint64*)value, andValue);
}


int64_t
fssh_atomic_or64(vint64_t *value, int64_t orValue)	
{
	return atomic_or64((vint64*)value, orValue);
}


int64_t
fssh_atomic_get64(vint64_t *value)
{
	return atomic_get64((vint64*)value);
}

