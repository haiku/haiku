/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include "syscalls.h"


thread_id
find_thread(const char* name)
{
	// TODO x86_64: x86 is doing some TLS thing here. Should that be done here
	// too?
	return _kern_find_thread(name);
}

