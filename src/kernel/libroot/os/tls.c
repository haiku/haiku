/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


// we don't want to have the inline assembly included here
#ifndef _NO_INLINE_ASM
#	define _NO_INLINE_ASM 1
#endif

#include "support/TLS.h"


// ToDo: we probably need to set some standard slots (like it's done in BeOS)!
// 	Slot 1 is obviously the thread ID, but there are 8 pre-allocated slots in BeOS.
//	I have no idea what the other slots are used for; they are all NULL when the
//	thread has just started, but slots 0 & 1.
//	Slot 0 seems to be an address somewhere on the stack. Another slot is most probably
//	reserved for "errno", the others could have been used to make some POSIX calls
//	thread-safe - but we also have to find out which supposely non-thread-safe POSIX
//	calls are thread-safe in BeOS.
static int32 gNextSlot = 0;


int32
tls_allocate(void)
{
	int32 next = atomic_add(&gNextSlot, 1);
	if (next >= TLS_MAX_KEYS)
		return B_NO_MEMORY;

	return next;
}


#if __INTEL__ && __GNUC__

void *
tls_get(int32 index)
{
	void *ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:(,%%edx, 4), %%eax \n\t"
		: "=a"(ret) : "d"(index) );
	return ret;
}


void **
tls_address(int32 index)
{
	void *ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:0, %%eax \n\t"
		"leal	(%%eax, %%edx, 4), %%eax \n\t"
		: "=a"(ret) : "d"(index) );
	return ret;
}


void
tls_set(int32 index, void *value)
{
	__asm__ __volatile__ ( 
		"movl	%%eax, %%fs:(,%%edx, 4) \n\t"
		: : "d"(index), "a"(value) );
}

#else
#	error "TLS has not been implemented for this platform"
#endif
