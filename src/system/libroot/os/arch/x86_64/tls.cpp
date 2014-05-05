/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#ifndef _NO_INLINE_ASM
#	define _NO_INLINE_ASM 1
#endif

#include <atomic>

#include <runtime_loader/runtime_loader.h>

#include <support/TLS.h>
#include <tls.h>

#include <assert.h>


struct tls_index {
	unsigned long int	module;
	unsigned long int	offset;
};


static std::atomic<int> gNextSlot(TLS_FIRST_FREE_SLOT);


static inline void**
get_tls()
{
	void** tls;
	__asm__ __volatile__ ("movq	%%fs:0, %0" : "=r" (tls));
	return tls;
}


int32
tls_allocate()
{
	if (gNextSlot < TLS_MAX_KEYS) {
		auto next = gNextSlot++;
		if (next < TLS_MAX_KEYS)
			return next;
	}

	return B_NO_MEMORY;
}


void*
tls_get(int32 index)
{
	return get_tls()[index];
}


void**
tls_address(int32 index)
{
	return get_tls() + index;
}


void
tls_set(int32 index, void* value)
{
	get_tls()[index] = value;
}


extern "C" void*
__tls_get_addr(tls_index* ti)
{
	return __gRuntimeLoader->get_tls_address(ti->module, ti->offset);
}

