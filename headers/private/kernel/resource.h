/*
** Copyright 2002, Jeff Hamilton. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__nulibc_sys_resource__hh__
#define __newos__nulibc_sys_resource__hh__

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
