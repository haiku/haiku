/*
 * Copyright 2019-2022, Haiku, Inc. All Rights Reserved.
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <runtime_loader/runtime_loader.h>

#include <support/TLS.h>
#include <tls.h>


struct tls_index {
	unsigned long ti_module;
	unsigned long ti_offset;
};


static int32 gNextSlot = TLS_FIRST_FREE_SLOT;


static inline void**
get_tls()
{
	void **tls;
	asm volatile("MRC p15, 0, %0, c13, c0, 3" : "=r" (tls));
	return tls;
}


int32
tls_allocate(void)
{
	int32 next = atomic_add(&gNextSlot, 1);
	if (next >= TLS_MAX_KEYS)
		return B_NO_MEMORY;

	return next;
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


void *
__tls_get_addr(struct tls_index *ti)
{
	return __gRuntimeLoader->get_tls_address(ti->ti_module, ti->ti_offset);
}
