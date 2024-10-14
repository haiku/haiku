/*
 * Copyright 2006-2024, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_PARAM_H_
#define _BSD_SYS_PARAM_H_


#include_next <sys/param.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#define MAXLOGNAME 33
#define NBBY 8


#define ALIGNBYTES	_ALIGNBYTES
#define ALIGN(p)	_ALIGN(p)

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define nitems(x)	(sizeof((x)) / sizeof((x)[0]))
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define rounddown(x, y)  (((x)/(y))*(y))
#define powerof2(x)	((((x)-1)&(x))==0)


#endif	/* _DEFAULT_SOURCE */


#endif	/* _BSD_SYS_PARAM_H_ */
