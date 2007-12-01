/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_PARAM_H_
#define _FBSD_COMPAT_SYS_PARAM_H_


#include <posix/sys/param.h>


/* The version this compatibility layer is based on */
#define __FreeBSD_version 700053


#define MAXBSIZE	0x10000

#define PAGE_SHIFT	12
#define PAGE_SIZE	B_PAGE_SIZE
#define PAGE_MASK	(PAGE_SIZE - 1)


#define trunc_page(x)	((x) & ~PAGE_MASK)


#define ptoa(x)			((unsigned long)((x) << PAGE_SHIFT))
#define atop(x)			((unsigned long)((x) >> PAGE_SHIFT))

/* MAJOR FIXME */
#define Maxmem			(32768)


#ifndef MSIZE
#define MSIZE 256
#endif

#ifndef MCLSHIFT
#define MCLSHIFT 11
#endif

#define MCLBYTES		(1 << MCLSHIFT)

#define	MJUMPAGESIZE	PAGE_SIZE
#define	MJUM9BYTES		(9 * 1024)
#define	MJUM16BYTES		(16 * 1024)

#define ALIGN_BYTES		(sizeof(int) - 1)
#define ALIGN(x)		((((unsigned)x) + ALIGN_BYTES) & ~ALIGN_BYTES)

#define roundup2(x, y)	(((x) + ((y) - 1)) & (~((y) - 1)))

#endif	/* _FBSD_COMPAT_SYS_PARAM_H_ */
