/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_BLOCK_CACHE_H
#define _KERNEL_BLOCK_CACHE_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern status_t block_cache_init(void);
extern size_t block_cache_used_memory();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_BLOCK_CACHE_H */
