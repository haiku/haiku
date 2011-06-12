/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_MEMORY_PRIVATE_H
#define _LIBROOT_MEMORY_PRIVATE_H

#include <OS.h>

#include <sys/cdefs.h>


__BEGIN_DECLS

status_t get_memory_properties(team_id teamID, const void* address,
	uint32* _protected, uint32* _lock);

__END_DECLS


#endif	// _LIBROOT_MEMORY_PRIVATE_H
