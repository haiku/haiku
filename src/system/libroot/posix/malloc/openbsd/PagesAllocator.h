/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PAGES_ALLOCATOR_H
#define _PAGES_ALLOCATOR_H


#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif


void __init_pages_allocator();
void __pages_allocator_before_fork();
void __pages_allocator_after_fork(int parent);


status_t __allocate_pages(void** address, size_t length, int flags);
status_t __free_pages(void* address, size_t length);


#ifdef __cplusplus
} // extern "C"
#endif


#endif	// _PAGES_ALLOCATOR_H
