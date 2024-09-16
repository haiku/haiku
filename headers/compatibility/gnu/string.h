/*
 * Copyright 2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_STRING_H_
#define _GNU_STRING_H_


#include_next <string.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

extern void *memmem(const void *source, size_t sourceLength,
	const void *search, size_t searchLength);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_STRING_H_ */
