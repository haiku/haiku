#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H
/* 
** Copyright 2002, Jeff Hamilton. All rights reserved.
** Distributed under the terms of the NewOS License.
** Distributed under the terms of the OpenBeOS License.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long	rlim_t;

struct rlimit {
	rlim_t	rlim_cur;	// Current soft limit
	rlim_t	rlim_max;	// System hard limit
};

#define RLIMIT_CORE		0
#define RLIMIT_CPU		1
#define	RLIMIT_DATA		2
#define RLIMIT_FSIZE	3
#define RLIMIT_NOFILE	4
#define RLIMIT_STACK	5
#define RLIMIT_AS		6
#define RLIMIT_VMEM		7

#define RLIM_INFINITY	(0xffffffffUL)

int getrlimit(int resource, struct rlimit * rlp);
int setrlimit(int resource, const struct rlimit * rlp);

#include <sys/time.h>

struct rusage {
	struct timeval ru_utime; // user time used
	struct timeval ru_stime; // system time used
};

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN -1

int getrusage(int processes, struct rusage *rusage);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
