/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_PARAM_H_
#define _BSD_SYS_PARAM_H_


#include_next <sys/param.h>


#ifndef _ALIGNBYTES
#	define _ALIGNBYTES 7
#endif
#ifndef _ALIGN
#	define _ALIGN(p) (((unsigned)(p) + _ALIGNBYTES) & ~_ALIGNBYTES)
#endif

#ifndef ALIGNBYTES
#	define ALIGNBYTES _ALIGNBYTES
#endif
#ifndef ALIGN
#	define ALIGN(p) _ALIGN(p)
#endif

#ifndef howmany
#	define howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#ifndef MAXLOGNAME
#	define MAXLOGNAME 32
#endif

#endif	/* _BSD_SYS_PARAM_H_ */
