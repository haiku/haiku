/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _CDEFS_H
#define _CDEFS_H

#ifdef __GNUC__
# define __PRINTFLIKE(__fmt,__varargs) __attribute__((__format__ (__printf__, __fmt, __varargs)))
# define __SCANFLIKE(__fmt,__varargs) __attribute__((__format__ (__scanf__, __fmt, __varargs)))
# define __PURE __attribute__((__const__))
#else
# define __PRINTFLIKE(__fmt,__varargs)
# define __SCANFLIKE(__fmt,__varargs)
# define __PURE
#endif

#endif
