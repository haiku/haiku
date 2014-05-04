/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


// we don't want to have the inline assembly included here
#ifndef _NO_INLINE_ASM
#	define _NO_INLINE_ASM 1
#endif

#include <runtime_loader/runtime_loader.h>

#include "support/TLS.h"
#include "tls.h"


#if !defined(__GNUC__) || (__GNUC__ > 2)

struct tls_index {
	unsigned long int	module;
	unsigned long int	offset;
};


void* ___tls_get_addr(struct tls_index* ti) __attribute__((__regparm__(1)));

#endif	// GCC2


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


#if !defined(__GNUC__) || (__GNUC__ > 2)


void* __attribute__((__regparm__(1)))
___tls_get_addr(struct tls_index* ti)
{
	return __gRuntimeLoader->get_tls_address(ti->module, ti->offset);
}


#endif	// GCC2

