/* 
** Copyright 2004, Jérôme Duval. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <time.h>
#include <OS.h>


int
stime(const time_t *tp)
{
	if (tp == NULL)
		return B_ERROR;
	set_real_time_clock(*tp);
	return B_OK;
}

