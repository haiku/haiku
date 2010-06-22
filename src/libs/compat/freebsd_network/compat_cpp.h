/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FREE_BSD_NETWORK_COMPAT_CPP_H
#define _FREE_BSD_NETWORK_COMPAT_CPP_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif


void*	_kernel_contigmalloc_cpp(const char* file, int line, size_t size,
			phys_addr_t low, phys_addr_t high, phys_size_t alignment,
			phys_size_t boundary, bool zero, bool dontWait);


#ifdef __cplusplus
}
#endif


#endif /* _FREE_BSD_NETWORK_COMPAT_CPP_H */
