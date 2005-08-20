/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include "syscalls.h"


thread_id
find_thread(const char *name)
{
	// BeOS R5 applications also use this trick as find_thread was available
	// in BeOS R5 OS.h as inline function. Do not change storage of thread id.
	if (!name) {
		thread_id thread;
		__asm__ __volatile__ ( 
			"movl	%%fs:4, %%eax \n\t"
			: "=a" (thread));
		return thread;
	}
	return _kern_find_thread(name);
}


// see OS.h from BeOS R5 for the reason why we need this
// there find_thread (see above) is provided as inline function
extern thread_id _kfind_thread_(const char *name);


thread_id
_kfind_thread_(const char *name)
{
	return _kern_find_thread(name);
}


extern thread_id _kget_thread_stacks_(thread_id thread, uint32 *stacks);

status_t
_kget_thread_stacks_(thread_id thread, uint32 *stacks)
{
	// This one is obviously called from the R4.5 startup code. I am not
	// exactly sure how it returns its infos, but just returning an
	// error seems to work fine as well
	return B_ERROR;
}

