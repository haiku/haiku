/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <time.h>
#include <SupportDefs.h>


double
difftime(time_t time1, time_t time2)
{
	return 1. * ((bigtime_t)time1 - (bigtime_t)time2);
}

