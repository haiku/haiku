/*
 * Copyright 2009-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_STRING_H_
#define _BSD_STRING_H_


#include_next <string.h>


#ifdef __cplusplus
extern "C" {
#endif

char* strsep(char** string, const char* delimiters);

#ifdef __cplusplus
}
#endif

#endif	/* _BSD_STRING_H_ */
