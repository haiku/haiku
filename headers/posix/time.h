/*
 * Copyright 2005-2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_H_
#define _TIME_H_


#include <sys/types.h>


typedef long clock_t;
typedef long time_t;
typedef long suseconds_t;
typedef unsigned long useconds_t;

#define CLOCKS_PER_SEC	1000
#define CLK_TCK			CLOCKS_PER_SEC

#define MAX_TIMESTR		70
	/* maximum length of a string returned by asctime(), and ctime() */

struct timespec {
	time_t	tv_sec;		/* seconds */
	long	tv_nsec;	/* and nanoseconds */
};

struct itimerspec {
	struct timespec it_interval;
	struct timespec it_value;
};

struct tm {
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


/* special timezone support */
extern char *tzname[2];
extern int 	daylight;
extern long	timezone;


#ifdef __cplusplus
extern "C" {
#endif

extern clock_t		clock(void);
extern double		difftime(time_t time1, time_t time2);
extern time_t		mktime(struct tm *tm);
extern time_t		time(time_t *timer);
extern char			*asctime(const struct tm *tm);
extern char			*asctime_r(const struct tm *timep, char *buffer);
extern char			*ctime(const time_t *timer);
extern char			*ctime_r(const time_t *timer, char *buffer);
extern struct tm	*gmtime(const time_t *timer);
extern struct tm	*gmtime_r(const time_t *timer, struct tm *tm); 
extern struct tm	*localtime(const time_t *timer);
extern struct tm	*localtime_r(const time_t *timer, struct tm *tm);
extern size_t		strftime(char *buffer, size_t maxSize, const char *format,
						const struct tm *tm);
extern char 		*strptime(const char *buf, const char *format, struct tm *tm);

/* special timezone support */
extern void tzset(void);
extern int	stime(const time_t *t);

#ifdef __cplusplus
}
#endif

#endif	/* _TIME_H_ */
