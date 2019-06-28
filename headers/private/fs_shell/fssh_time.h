/*
 * Copyright 2005-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_TIME_H_
#define _FSSH_TIME_H_


#include "fssh_defs.h"


#if defined(__i386__) && !defined(__x86_64__)
typedef int32_t fssh_time_t;
#else
typedef int64_t fssh_time_t;
#endif

typedef int32_t fssh_clock_t;
typedef int32_t fssh_suseconds_t;
typedef uint32_t fssh_useconds_t;

#define FSSH_CLOCKS_PER_SEC	1000
#define FSSH_CLK_TCK		FSSH_CLOCKS_PER_SEC

#define FSSH_MAX_TIMESTR	70
	/* maximum length of a string returned by asctime(), and ctime() */

struct fssh_timespec {
	fssh_time_t	tv_sec;		/* seconds */
	long		tv_nsec;	/* and nanoseconds */
};

struct fssh_itimerspec {
	struct fssh_timespec it_interval;
	struct fssh_timespec it_value;
};

struct fssh_tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;	/* day of month (1 to 31) */
	int	tm_mon;		/* months since January (0 to 11) */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday (0 to 6, Sunday = 0, ...) */
	int	tm_yday;	/* days since January 1 (0 to 365) */
	int	tm_isdst;	/* daylight savings time (0 == no, >0 == yes, <0 == has to be calculated */
	int tm_gmtoff;	/* timezone offset to GMT */
	char *tm_zone;	/* timezone name */
};


#ifdef __cplusplus
extern "C" {
#endif

/* special timezone support */
extern char *fssh_tzname[2];
extern int 	fssh_daylight;
extern long	fssh_timezone;

extern fssh_clock_t		fssh_clock(void);
extern double			fssh_difftime(fssh_time_t time1, fssh_time_t time2);
extern fssh_time_t		fssh_mktime(struct fssh_tm *tm);
extern fssh_time_t		fssh_time(fssh_time_t *timer);
extern char				*fssh_asctime(const struct fssh_tm *tm);
extern char				*fssh_asctime_r(const struct fssh_tm *timep,
								char *buffer);
extern 	char			*fssh_ctime(const fssh_time_t *timer);
extern char				*fssh_ctime_r(const fssh_time_t *timer, char *buffer);
extern struct fssh_tm	*fssh_gmtime(const fssh_time_t *timer);
extern struct fssh_tm	*fssh_gmtime_r(const fssh_time_t *timer,
								struct fssh_tm *tm);
extern struct fssh_tm	*fssh_localtime(const fssh_time_t *timer);
extern struct fssh_tm	*fssh_localtime_r(const fssh_time_t *timer,
								struct fssh_tm *tm);
extern fssh_size_t		fssh_strftime(char *buffer, fssh_size_t maxSize,
								const char *format, const struct fssh_tm *tm);
extern char 			*fssh_strptime(const char *buf, const char *format,
								struct fssh_tm *tm);

/* special timezone support */
extern void fssh_tzset(void);
extern int	fssh_stime(const fssh_time_t *t);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_TIME_H_ */
