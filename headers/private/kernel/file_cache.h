/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_FILE_CACHE_H
#define _KERNEL_FILE_CACHE_H


#include <vm_types.h>


#ifdef __cplusplus
extern "C" {
#endif

extern vm_store *vm_create_vnode_store(void *vnode);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_FILE_CACHE_H */
