/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include <OS.h>

#include "fssh_atomic.h"


void
fssh_atomic_set(int32_t* value, int32_t newValue)
{
	atomic_set((int32*)value, newValue);
}


int32_t
fssh_atomic_get_and_set(int32_t* value, int32_t newValue)
{
	return atomic_get_and_set((int32*)value, newValue);
}


int32_t
fssh_atomic_test_and_set(int32_t *value, int32_t newValue, int32_t testAgainst)
{
	return atomic_test_and_set((int32*)value, newValue, testAgainst);
}


int32_t
fssh_atomic_add(int32_t *value, int32_t addValue)
{
	return atomic_add((int32*)value, addValue);
}


int32_t
fssh_atomic_and(int32_t *value, int32_t andValue)
{
	return atomic_and((int32*)value, andValue);
}


int32_t
fssh_atomic_or(int32_t *value, int32_t orValue)	
{
	return atomic_or((int32*)value, orValue);
}


int32_t
fssh_atomic_get(int32_t *value)
{
	return atomic_get((int32*)value);
}


void
fssh_atomic_set64(int64_t *value, int64_t newValue)
{
	atomic_set64((int64*)value, newValue);
}


int64_t
fssh_atomic_get_and_set64(int64_t* value, int64_t newValue)
{
	return atomic_get_and_set64((int64*)value, newValue);
}


int64_t
fssh_atomic_test_and_set64(int64_t *value, int64_t newValue, int64_t testAgainst)
{
	return atomic_test_and_set64((int64 *)value, newValue, testAgainst);
}


int64_t
fssh_atomic_add64(int64_t *value, int64_t addValue)
{
	return atomic_add64((int64*)value, addValue);
}


int64_t
fssh_atomic_and64(int64_t *value, int64_t andValue)
{
	return atomic_and64((int64*)value, andValue);
}


int64_t
fssh_atomic_or64(int64_t *value, int64_t orValue)	
{
	return atomic_or64((int64*)value, orValue);
}


int64_t
fssh_atomic_get64(int64_t *value)
{
	return atomic_get64((int64*)value);
}

