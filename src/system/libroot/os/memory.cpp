/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include <memory_private.h>

#include <OS.h>

#include <syscalls.h>


status_t
get_memory_properties(team_id teamID, const void* address, uint32* _protected,
	 uint32* _lock)
{
	return _kern_get_memory_properties(teamID, address, _protected, _lock);
}
