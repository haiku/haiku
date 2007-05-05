#ifndef _FBSD_COMPAT_SYS_PARAM_H_
#define _FBSD_COMPAT_SYS_PARAM_H_

#include <posix/sys/param.h>

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

#define MCLBYTES	(1 << MCLSHIFT)

#define ALIGN_BYTES		(sizeof(int) - 1)
#define ALIGN(x)		((((unsigned)x) + ALIGN_BYTES) & ~ALIGN_BYTES)

#endif
