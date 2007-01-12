/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_STORE_ANONYMOUS_H
#define _KERNEL_VM_STORE_ANONYMOUS_H


#include <vm_types.h>


#ifdef __cplusplus
extern "C" {
#endif

vm_store *vm_store_create_anonymous_noswap(bool canOvercommit,
	int32 numPrecommittedPages, int32 numGuardPages);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_STORE_ANONYMOUS_H */
