/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_TLS_H
#define _KERNEL_TLS_H


#include <support/TLS.h>


#define TLS_SIZE (TLS_MAX_KEYS * sizeof(void *))

enum {
	TLS_BASE_ADDRESS_SLOT = 0,
		// contains the address of the local storage space

	TLS_THREAD_ID_SLOT,
	TLS_ERRNO_SLOT,
	TLS_ON_EXIT_THREAD_SLOT,
	TLS_USER_THREAD_SLOT,

	// Note: these entries can safely be changed between
	// releases; 3rd party code always calls tls_allocate()
	// to get a free slot

	TLS_FIRST_FREE_SLOT
		// the first free slot for user allocations
};

#endif	/* _KERNEL_TLS_H */
