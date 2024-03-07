/*
 * Copyright 2016-2017 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_TIME_H_
#define _BSD_SYS_TIME_H_


#include_next <sys/time.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


/* BSDish macros operating on timespec structs */
#define	timespecadd(a, b, res)							\
	do {												\
		(res)->tv_sec = (a)->tv_sec + (b)->tv_sec;		\
		(res)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;	\
		if ((res)->tv_nsec >= 1000000000L) {			\
			(res)->tv_nsec -= 1000000000L;				\
			(res)->tv_sec++;							\
		}												\
	} while (0)
#define	timespecsub(a, b, res)							\
	do {												\
		(res)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
		(res)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;	\
		if ((res)->tv_nsec < 0) {						\
			(res)->tv_nsec += 1000000000L;				\
			(res)->tv_sec--;							\
		}												\
	} while (0)
#define	timespecclear(a)	((a)->tv_sec = (a)->tv_nsec = 0)
#define	timespecisset(a)	((a)->tv_sec != 0 || (a)->tv_nsec != 0)
#define	timespeccmp(a, b, cmp)	(((a)->tv_sec == (b)->tv_sec) \
	? ((a)->tv_nsec cmp (b)->tv_nsec) : ((a)->tv_sec cmp (b)->tv_sec))
#define	timespecvalid_interval(a)	((a)->tv_sec >= 0	\
	&& (a)->tv_nsec >= 0 && (&)->tv_nsec < 1000000000L)


#ifdef __cplusplus
extern "C" {
#endif

int  lutimes(const char *path, const struct timeval times[2]);

#ifdef __cplusplus
}
#endif


#endif


#endif  /* _BSD_SYS_TIME_H_ */
