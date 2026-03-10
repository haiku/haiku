/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H


#include_next <string.h>


#if defined(__GNUC__) && __GNUC__ >= 4
#define memset(DEST, V, LEN)	__builtin_memset((DEST), (V), (LEN))
#define memcpy(DEST, SRC, LEN)	__builtin_memcpy((DEST), (SRC), (LEN))
#define memmove(DEST, SRC, LEN)	__builtin_memmove((DEST), (SRC), (LEN))
#define memcmp(B1, B2, LEN)		__builtin_memcmp((B1), (B2), (LEN))
#endif


#endif	/* KERNEL_STRING_H */
