/*
 * Copyright 2012-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval <jerome.duval@free.fr>
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <OS.h>

#include <arch_atomic.h>
#include "syscalls.h"


thread_id
find_thread(const char *name)
{
	return _kern_find_thread(name);
}


/*
 * Fill out gcc __sync_synchronize built-in for ARM
 */
void
__sync_synchronize(void)
{
	dmb();
}
