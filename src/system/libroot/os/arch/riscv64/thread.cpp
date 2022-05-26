/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include "syscalls.h"
#include <tls.h>


thread_id
find_thread(const char *name)
{
	if (name == NULL)
		return (thread_id)(addr_t)tls_get(TLS_THREAD_ID_SLOT);

	return _kern_find_thread(name);
}
