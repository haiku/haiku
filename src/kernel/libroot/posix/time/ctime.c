/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>


char *
ctime(const time_t *_timer)
{
	return asctime(localtime(_timer));
}


char *
ctime_r(const time_t *_timer, char *buf)
{
	struct tm tm;
	return asctime_r(localtime_r(_timer, &tm), buf);
}

