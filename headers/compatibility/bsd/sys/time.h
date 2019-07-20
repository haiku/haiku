/*
 * Copyright 2016-2017 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_TIME_H_
#define _BSD_SYS_TIME_H_


#include_next <sys/time.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

int  lutimes(const char *path, const struct timeval times[2]);

#ifdef __cplusplus
}
#endif


#endif


#endif  /* _BSD_SYS_TIME_H_ */
