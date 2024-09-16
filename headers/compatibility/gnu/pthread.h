/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_PTHREAD_H_
#define _GNU_PTHREAD_H_


#include_next <pthread.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#include <sched.h>


#ifdef __cplusplus
extern "C" {
#endif


#define PTHREAD_MAX_NAMELEN_NP	32	// B_OS_NAME_LENGTH


extern int pthread_getattr_np(pthread_t thread, pthread_attr_t* attr);

extern int pthread_getname_np(pthread_t thread, char* buffer, size_t length);
extern int pthread_setname_np(pthread_t thread, const char* name);

extern int pthread_timedjoin_np(pthread_t thread, void** _value, const struct timespec* abstime);

extern int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpuset_t* mask);
extern int pthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpuset_t* mask);


#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_PTHREAD_H_ */

