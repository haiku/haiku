/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


// see OS.h for the reason why we need this
thread_id _kfind_thread_(const char *name);

thread_id
_kfind_thread_(const char *name)
{
	return _kern_find_thread(name);
}
