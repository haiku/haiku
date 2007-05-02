#ifndef _FBSD_COMPAT_SYS_PARAM_H_
#define _FBSD_COMPAT_SYS_PARAM_H_

#include <posix/sys/param.h>

#define MAXBSIZE	0x10000
#define PAGE_SIZE	B_PAGE_SIZE

#ifndef MSIZE
#define MSIZE 256
#endif

#ifndef MCLSHIFT
#define MCLSHIFT 11
#endif

#define MCLBYTES	(1 << MCLSHIFT)

#endif
