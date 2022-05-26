/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Isaac Turner, turner.isaac@gmail.com
 *		Jacob Secunda, secundaja@gmail.com
 */
#ifndef _GNU_STDLIB_H_
#define _GNU_STDLIB_H_


#include_next <stdlib.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*_compare_function_qsort_r)(const void*, const void*, void*);

extern void qsort_r(void* base, size_t numElements, size_t sizeOfElement,
	_compare_function_qsort_r, void* cookie);

#ifdef __cplusplus
}
#endif


#endif /* _GNU_SOURCE */


#endif /* _GNU_STDLIB_H_ */
