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
		__asm__ __volatile__ ("movq %%fs:8, %%rax" : "=a" (thread));
		return thread;
	}

	return _kern_find_thread(name);
}

