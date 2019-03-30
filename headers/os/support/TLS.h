/*
 * Copyright 2003-2007 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TLS_H
#define _TLS_H


#include <BeBuild.h>
#include <SupportDefs.h>


/* A maximum of 64 keys is allowed to store in TLS - the key is reserved
 * process-wide. Note that tls_allocate() will return B_NO_MEMORY if you
 * try to exceed this limit.
 */
#define TLS_MAX_KEYS 64


#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

extern int32 tls_allocate(void);

#if !_NO_INLINE_ASM && __i386__ && __GNUC__

static inline void *
tls_get(int32 index)
{
	void *ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:(, %1, 4), %0"
		: "=r" (ret) : "r" (index));
	return ret;
}

static inline void **
tls_address(int32 index)
{
	void **ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:0, %0\n\t"
		"leal	(%0, %1, 4), %0\n\t"
		: "=&r" (ret) : "r" (index));
	return ret;
}

static inline void
tls_set(int32 index, void *value)
{
	__asm__ __volatile__ ( 
		"movl	%1, %%fs:(, %0, 4)"
		: : "r" (index), "r" (value));
}

#else	/* !_NO_INLINE_ASM && __i386__ && __GNUC__ */

extern void *tls_get(int32 index);
extern void **tls_address(int32 index);
extern void tls_set(int32 index, void *value);

#endif	/* !_NO_INLINE_ASM && __i386__ && __GNUC__ */

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* _TLS_H */
