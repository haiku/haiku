/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include "syscalls.h"


thread_id
find_thread(const char* name)
{
	if (!name) {
		thread_id thread;
		static_assert(sizeof(thread_id) <= sizeof(uint32_t),
			"thread_id is larger than uint32_t");
		__asm__ __volatile__ ("movl %%fs:8, %0" : "=r" (thread));
		return thread;
	}

	return _kern_find_thread(name);
}

