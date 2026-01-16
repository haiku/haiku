static inline const char*
__tm_to_tzname(const struct tm *tm)
{
	const void *p = tm->tm_zone;
	if (p)
		return p;
	if (tm->tm_gmtoff == timezone)
		return tzname[0];
	if (tm->tm_gmtoff == daylight)
		return tzname[1];
	if (tm->tm_gmtoff == 0)
		return "GMT";

	return "";
}

#define __tm_gmtoff tm_gmtoff
#define __tm_zone tm_zone
