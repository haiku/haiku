/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <time.h>


char *
ctime(const time_t *_timer)
{
	return asctime(localtime(_timer));
}
