/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include "syscalls.h"


thread_id
find_thread(const char *name)
{
	return _kern_find_thread(name);
}
