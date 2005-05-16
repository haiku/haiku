/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_STORE_NULL_H
#define _KERNEL_VM_STORE_NULL_H


#include <vm_types.h>


#ifdef __cplusplus
extern "C"
#endif
vm_store *vm_store_create_null(void);

#endif	/* _KERNEL_VM_STORE_NULL_H */
