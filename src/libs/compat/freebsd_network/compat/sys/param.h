/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_PARAM_H_
#define _FBSD_COMPAT_SYS_PARAM_H_


#include <posix/sys/param.h>

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/priority.h>


/* The version this compatibility layer is based on */
#define __FreeBSD_version 800107

#define MAXBSIZE	0x10000

#define PAGE_SHIFT	12
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

/* Macros for counting and rounding. */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))  /* to any y */
#define roundup2(x, y)	(((x) + ((y) - 1)) & (~((y) - 1)))
#define rounddown(x, y)  (((x) / (y)) * (y))

#define	PRIMASK	0x0ff
#define	PCATCH	0x100
#define	PDROP	0x200
#define	PBDRY	0x400

#define	NBBY	8		/* number of bits in a byte */

/* Bit map related macros. */
#define	setbit(a,i)	(((unsigned char *)(a))[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	(((unsigned char *)(a))[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)							\
	(((const unsigned char *)(a))[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)							\
	((((const unsigned char *)(a))[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

#endif	/* _FBSD_COMPAT_SYS_PARAM_H_ */
