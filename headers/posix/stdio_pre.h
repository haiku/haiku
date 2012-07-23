/*
 * Copyright 2003-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STDIO_PRE_H_
#define _STDIO_PRE_H_


#ifndef _STDIO_H_
#	error "This file must be included from stdio.h!"
#endif

#include <libio.h>

typedef struct _IO_FILE FILE;

#define __PRINTFLIKE(format, varargs) __attribute__ ((__format__ (__printf__, format, varargs)))
#define __SCANFLIKE(format, varargs) __attribute__((__format__ (__scanf__, format, varargs)))

#endif	/* _STDIO_PRE_H_ */
