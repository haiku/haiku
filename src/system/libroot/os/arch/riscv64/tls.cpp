/*
 * Copyright 2019-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <runtime_loader/runtime_loader.h>

#include "support/TLS.h"
#include "tls.h"


struct tls_index {
	unsigned long ti_module;
	unsigned long ti_offset;
};

extern "C" void* __tls_get_addr(struct tls_index* ti);

// TODO: use std::atomic<int> here like x86_64
static int32 gNextSlot = TLS_FIRST_FREE_SLOT;


static inline void**
get_tls()
{
	void** tls;
	__asm__ __volatile__ ("mv	%0, tp" : "=r" (tls));
	return tls;
}


int32
tls_allocate(void)
{
	if (gNextSlot < TLS_MAX_KEYS) {
		int32 next = gNextSlot++;
		if (next < TLS_MAX_KEYS)
			return next;
	}

	return B_NO_MEMORY;
}


void *
tls_get(int32 index)
{
	return get_tls()[index];
}


void **
tls_address(int32 index)
{
	return get_tls() + index;
}


void
tls_set(int32 index, void *value)
{
	get_tls()[index] = value;
}


void*
__tls_get_addr(struct tls_index* ti)
{
	return __gRuntimeLoader->get_tls_address(ti->ti_module, ti->ti_offset);
}
