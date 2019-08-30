/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_PTHREAD_H_
#define _GNU_PTHREAD_H_


#include_next <pthread.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif


extern int pthread_getattr_np(pthread_t thread, pthread_attr_t* attr);


#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_PTHREAD_H_ */

