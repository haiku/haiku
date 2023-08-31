/* The following methods were adapted from musl.
 * See musl/COPYRIGHT for license information. */

#include <errno.h>

#include "../musl/time/time_impl.h"


static long  __timezone = 0;
static int dst_off = 0;
const char __utc[] = "UTC";
static const char *__tzname[2] = { __utc, __utc };


static void
__secs_to_zone(long long t, int local, int *isdst, int *offset, long *oppoff, const char **zonename)
{
	*isdst = 0;
	*offset = -__timezone;
	if (oppoff) *oppoff = -dst_off;
	*zonename = __tzname[0];
}


struct tm *
__gmtime_r_fallback(const time_t *restrict t, struct tm *restrict tm)
{
	if (__secs_to_tm(*t, tm) < 0) {
		errno = EOVERFLOW;
		return 0;
	}
	tm->tm_isdst = 0;
	tm->__tm_gmtoff = 0;
	tm->__tm_zone = __utc;
	return tm;
}


time_t
__mktime_fallback(struct tm *tm)
{
	struct tm new;
	long opp;
	long long t = __tm_to_secs(tm);

	__secs_to_zone(t, 1, &new.tm_isdst, &new.__tm_gmtoff, &opp, &new.__tm_zone);

	if (tm->tm_isdst>=0 && new.tm_isdst!=tm->tm_isdst)
		t -= opp - new.__tm_gmtoff;

	t -= new.__tm_gmtoff;
	if ((time_t)t != t) goto error;

	__secs_to_zone(t, 0, &new.tm_isdst, &new.__tm_gmtoff, &opp, &new.__tm_zone);

	if (__secs_to_tm(t + new.__tm_gmtoff, &new) < 0) goto error;

	*tm = new;
	return t;

error:
	errno = EOVERFLOW;
	return -1;
}


time_t
__timegm_fallback(struct tm *tm)
{
	struct tm new;
	long long t = __tm_to_secs(tm);
	if (__secs_to_tm(t, &new) < 0) {
		errno = EOVERFLOW;
		return -1;
	}
	*tm = new;
	tm->tm_isdst = 0;
	tm->__tm_gmtoff = 0;
	tm->__tm_zone = __utc;
	return t;
}
