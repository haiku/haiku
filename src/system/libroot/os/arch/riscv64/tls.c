/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <runtime_loader/runtime_loader.h>

#include "support/TLS.h"
#include "tls.h"


struct tls_index {
	unsigned long ti_module;
	unsigned long ti_offset;
};

void* __tls_get_addr(struct tls_index* ti);

static int32 gNextSlot = TLS_FIRST_FREE_SLOT;


// TODO: RISCV64 has a dedicated (TP) register for TLS

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
	debugger("Implement TLS support before SMP");
	return NULL;
}


void **
tls_address(int32 index)
{
	debugger("Implement TLS support before SMP");
	return NULL;
}


void
tls_set(int32 index, void *value)
{
	debugger("Implement TLS support before SMP");
}


void*
__tls_get_addr(struct tls_index* ti)
{
	return __gRuntimeLoader->get_tls_address(ti->ti_module, ti->ti_offset);
}
