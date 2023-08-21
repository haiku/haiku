/*
* Copyright 2023 Haiku, Inc. All Rights Reserved.
* Distributed under the terms of the MIT License.
*/
#ifndef _GNU_SCHED_H_
#define _GNU_SCHED_H_


#include_next <sched.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

extern int sched_getcpu(void);

#ifdef __cplusplus
}
#endif


#endif


#endif  /* _GNU_SCHED_H_ */
