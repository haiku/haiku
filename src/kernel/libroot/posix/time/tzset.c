/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <time.h>


char *tzname[2];


void
tzset(void)
{
	// ToDo: implement me properly!

	tzname[0] = "std";
	tzname[0] = "dst";
}

