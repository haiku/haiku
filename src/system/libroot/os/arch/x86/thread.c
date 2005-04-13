/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include "syscalls.h"


// see OS.h from BeOS R5 for the reason why we need this
// (it's referenced there in the same way we reference _kern_find_thread())
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

