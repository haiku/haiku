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


int32
tls_allocate(void)
{
	assert(0 && "tls_allocate: not implemented");
	return 0;
}


void*
tls_get(int32 index)
{
	assert(0 && "tls_get: not implemented");
	return NULL;
}


void**
tls_address(int32 index)
{
	assert(0 && "tls_address: not implemented");
	return NULL;
}


void
tls_set(int32 index, void* value)
{
	assert(0 && "tls_set: not implemented");
}

