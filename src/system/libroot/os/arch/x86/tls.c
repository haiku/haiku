/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


// we don't want to have the inline assembly included here
#ifndef _NO_INLINE_ASM
#	define _NO_INLINE_ASM 1
#endif

#include "support/TLS.h"
#include "tls.h"


static int32 gNextSlot = TLS_FIRST_FREE_SLOT;


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
	void *ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:(, %1, 4), %0"
		: "=r" (ret) : "r" (index));
	return ret;
}


void **
tls_address(int32 index)
{
	void **ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:0, %0\n\t"
		"leal	(%0, %1, 4), %0\n\t"
		: "=&r" (ret) : "r" (index));
	return ret;
}


void
tls_set(int32 index, void *value)
{
	__asm__ __volatile__ ( 
		"movl	%1, %%fs:(, %0, 4)"
		: : "r" (index), "r" (value));
}

