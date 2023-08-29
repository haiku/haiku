/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_RANDOM_DEFS_H
#define _KERNEL_RANDOM_DEFS_H


#include <stddef.h>


#define RANDOM_SYSCALLS			"random"
#define RANDOM_GET_ENTROPY		1


struct random_get_entropy_args {
	void* buffer;
	size_t length;
};


#endif	/* _KERNEL_RANDOM_DEFS_H */
