/*
 * Copyright 2009-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_WAIT_H_
#define _BSD_SYS_WAIT_H_


#include_next <sys/wait.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#include <sys/resource.h>


#ifdef __cplusplus
extern "C" {
#endif

pid_t wait3(int *status, int options, struct rusage *rusage);

pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_SYS_WAIT_H_ */
