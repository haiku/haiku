#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/time.h>


/* getrlimit()/setrlimit() definitions */

typedef unsigned long rlim_t;

struct rlimit {
	rlim_t	rlim_cur;		/* current soft limit */
	rlim_t	rlim_max;		/* hard limit */
};

// ToDo: the only supported mode is RLIMIT_NOFILE right now
#define RLIMIT_CORE		0	/* size of the core file */
#define RLIMIT_CPU		1	/* CPU time per team */
#define	RLIMIT_DATA		2	/* data segment size */
#define RLIMIT_FSIZE	3	/* file size */
#define RLIMIT_NOFILE	4	/* number of open files */
#define RLIMIT_STACK	5	/* stack size */
#define RLIMIT_AS		6	/* address space size */

#define RLIM_INFINITY	(0xffffffffUL)
#define RLIM_SAVED_MAX	RLIM_INFINITY
#define RLIM_SAVED_CUR	RLIM_INFINITY


/* getrusage() definitions */

struct rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
};

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN -1


/* getpriority()/setpriority() definitions */

#define PRIO_PROCESS	0
#define PRIO_PGRP		1
#define PRIO_USER		2


#ifdef __cplusplus
extern "C" {
#endif

extern int getrusage(int processes, struct rusage *rusage);

extern int getrlimit(int resource, struct rlimit * rlp);
extern int setrlimit(int resource, const struct rlimit * rlp);

// ToDo: The following POSIX calls are missing (in BeOS as well):
//int getpriority(int which, id_t who);
//int setpriority(int which, id_t who, int priority);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_RESOURCE_H */
