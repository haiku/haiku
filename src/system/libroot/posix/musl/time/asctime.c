#include <time.h>

char *asctime(const struct tm *tm)
{
	static char buf[26];
	return asctime_r(tm, buf);
}
