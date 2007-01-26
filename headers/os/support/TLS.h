/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TLS_H
#define	_TLS_H


#include <BeBuild.h>
#include <SupportDefs.h>


/* A maximum of 64 keys is allowed to store in TLS - the key is reserved
 * process-wide. Note that tls_allocate() will return B_NO_MEMORY if you
 * try to exceed this limit.
 */
#define TLS_MAX_KEYS 64


#ifdef __cplusplus
extern "C" {
#endif

extern int32 tls_allocate(void);

#if !_NO_INLINE_ASM && __INTEL__ && __GNUC__

static inline void *
tls_get(int32 index)
{
	void *ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:(,%%edx, 4), %%eax \n\t"
		: "=a"(ret) : "d"(index) );
	return ret;
}

static inline void **
tls_address(int32 index)
{
	void **ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:0, %%eax \n\t"
		"leal	(%%eax, %%edx, 4), %%eax \n\t"
		: "=a"(ret) : "d"(index) );
	return ret;
}

static inline void
tls_set(int32 index, void *value)
{
	__asm__ __volatile__ ( 
		"movl	%%eax, %%fs:(,%%edx, 4) \n\t"
		: : "d"(index), "a"(value) );
}

#else

extern void *tls_get(int32 index);
extern void **tls_address(int32 index);
extern void tls_set(int32 index, void *value);

#endif

#ifdef __cplusplus
}
#endif

#endif	/* _TLS_H */
