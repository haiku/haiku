/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#ifndef _NO_INLINE_ASM
#	define _NO_INLINE_ASM 1
#endif

#include <support/TLS.h>
#include <tls.h>

#include <assert.h>


static int32 gNextSlot = TLS_FIRST_FREE_SLOT;


int32
tls_allocate(void)
{
	int32 next = atomic_add(&gNextSlot, 1);
	if (next >= TLS_MAX_KEYS)
		return B_NO_MEMORY;

	return next;
}


void*
tls_get(int32 _index)
{
	int64 index = _index;
	void* ret;

	__asm__ __volatile__ (
		"movq	%%fs:(, %1, 8), %0"
		: "=r" (ret) : "r" (index));
	return ret;
}


void**
tls_address(int32 _index)
{
	int64 index = _index;
	void** ret;

	__asm__ __volatile__ (
		"movq	%%fs:0, %0\n\t"
		"leaq	(%0, %1, 8), %0\n\t"
		: "=&r" (ret) : "r" (index));
	return ret;
}


void
tls_set(int32 _index, void* value)
{
	int64 index = _index;
	__asm__ __volatile__ (
		"movq	%1, %%fs:(, %0, 8)"
		: : "r" (index), "r" (value));
}
