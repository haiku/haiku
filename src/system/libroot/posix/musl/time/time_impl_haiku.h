time_t	timegm(struct tm *tm);

static inline time_t
__tm_to_secs(const struct tm *tm)
{
	struct tm tmp = *tm;
	return timegm(&tmp);
}

static inline const char*
__tm_to_tzname(const struct tm *tm)
{
	const void *p = tm->tm_zone;
	if (p)
		return p;
	if (tm->tm_isdst >= 0)
		return tzname[tm->tm_isdst != 0];

	return "";
}

#define __tm_gmtoff tm_gmtoff
#define __tm_zone tm_zone
