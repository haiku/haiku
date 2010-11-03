/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_STDIO_H_
#define _BSD_STDIO_H_


#include_next <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

char *fgetln(FILE *stream, size_t *_length);

#ifdef __cplusplus
}
#endif

#endif	/* _BSD_STDIO_H_ */
