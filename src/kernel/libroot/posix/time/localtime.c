/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <time.h>
#include <stdio.h>


struct tm *
localtime_r(const time_t *_timer, struct tm *tm)
{
	return tm;
}


struct tm *
localtime(const time_t *_timer)
{
	static struct tm tm;
	return localtime_r(_timer, &tm);
}

