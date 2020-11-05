/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_STDIO_H_
#define _BSD_STDIO_H_


#include_next <stdio.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

char *fgetln(FILE *stream, size_t *_length);

int asprintf(char **ret, char const *format, ...) __PRINTFLIKE(2,3);
int vasprintf(char **ret, char const *format, va_list ap);


#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_STDIO_H_ */
