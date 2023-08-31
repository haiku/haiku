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
