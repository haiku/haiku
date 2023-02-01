/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_MALLOC_H_
#define _OBSD_COMPAT_SYS_MALLOC_H_


#include_next <sys/malloc.h>


#define malloc(size, base, flags)	kernel_malloc(size, base, flags)
#define free(addr, type, freedsize) _kernel_free(addr)


#define M_CANFAIL (0)


#endif	/* _OBSD_COMPAT_SYS_MALLOC_H_ */
