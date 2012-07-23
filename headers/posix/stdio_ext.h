/*
 * Copyright 2008-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STDIO_EXT_H_
#define _STDIO_EXT_H_


#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSETLOCKING_QUERY		0
#define FSETLOCKING_INTERNAL	1
#define FSETLOCKING_BYCALLER	2

/* The following stdio extensions are not implemented yet */
/* extern size_t	__fufsize(FILE* stream); */
extern int		__freading(FILE* stream);
/* extern int		__fwriting(FILE* stream); */
/* extern int		__freadable(FILE* stream); */
/* extern int		__fwritable(FILE* stream); */
/* extern int		__flbf(FILE* stream); */
extern void		__fpurge(FILE* stream);
/* extern size_t	__fpending(FILE* stream); */

extern void		_flushlbf(void);
extern int		__fsetlocking(FILE* stream, int type);

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_EXT_H_ */

