/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef _SLAB_PRIVATE_H_
#define _SLAB_PRIVATE_H_

#include <stddef.h>

void *block_alloc(size_t size);
void block_free(void *block);
void block_allocator_init_boot();
void block_allocator_init_rest();

#endif

