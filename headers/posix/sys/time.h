#ifndef _SYS_TIME_H
#define _SYS_TIME_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/types.h>


struct timeval {
	time_t		tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* microseconds */
};

#include <sys/select.h>
	/* circular dependency: fd_set needs to be defined and the
	 * select prototype exported by this file, but <sys/select.h>
	 * needs struct timeval.
	 */

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

#define ITIMER_REAL		1	/* real time */
#define ITIMER_VIRTUAL	2	/* process virtual time */
#define ITIMER_PROF		3	/* both */


#ifdef __cplusplus
extern "C" {
#endif

extern int	getitimer(int which, struct itimerval *value);
extern int	setitimer(int which, const struct itimerval *value, struct itimerval *oldValue);
extern int	gettimeofday(struct timeval *tv, struct timezone *tz);

extern int	utimes(const char *name, const struct timeval times[2]);
	/* legacy */

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_TIME_H */
