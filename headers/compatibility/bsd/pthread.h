/*
 * Copyright 2023 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_PTHREAD_H_
#define _BSD_PTHREAD_H_


#include_next <pthread.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif


extern int pthread_attr_get_np(pthread_t thread, pthread_attr_t* attr);


#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_PTHREAD_H_ */

