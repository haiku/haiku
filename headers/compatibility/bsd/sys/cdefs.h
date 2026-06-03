/*
 * Copyright 2006-2012 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_CDEFS_H_
#define _BSD_SYS_CDEFS_H_


#include_next <sys/cdefs.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#define __FBSDID(x)
#define	__unused	__attribute__((__unused__))
#define __packed	__attribute__((packed))
#define	__aligned(x) __attribute__((__aligned__(x)))


#if __GNUC__ > 4
#define __predict_true(exp)     __builtin_expect((exp), 1)
#define __predict_false(exp)    __builtin_expect((exp), 0)
#define	__unreachable()			__builtin_unreachable()
#else
#define __predict_true(exp)     (exp)
#define __predict_false(exp)    (exp)
#define	__unreachable()			((void)0)
#endif


#define __printflike(a, b)	__attribute__ ((format (__printf__, (a), (b))))
#define __printf0like(a, b)


#endif


#endif	/* _BSD_SYS_CDEFS_H_ */
