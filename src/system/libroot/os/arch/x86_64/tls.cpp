/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


// TODO x86_64.
// Also want to add inline versions to support/TLS.h.


#ifndef _NO_INLINE_ASM
#	define _NO_INLINE_ASM 1
#endif

#include <support/TLS.h>
#include <tls.h>

#include <assert.h>


static int32 gNextSlot = TLS_FIRST_FREE_SLOT;
static void* gSlots[TLS_MAX_KEYS];


int32
tls_allocate(void)
{
	int32 next = atomic_add(&gNextSlot, 1);
	if (next >= TLS_MAX_KEYS)
		return B_NO_MEMORY;

	return next;
}


void*
tls_get(int32 index)
{
	return gSlots[index];
}


void**
tls_address(int32 index)
{
	return &gSlots[index];
}


void
tls_set(int32 index, void* value)
{
	gSlots[index] = value;
}
