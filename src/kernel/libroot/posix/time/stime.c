/* 
** Copyright 2004, Jérôme Duval. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <time.h>
#include <OS.h>
#include <errno.h>

// ToDo: replace zero by a ROOT_UID when usergroup.c is implemented

int
stime(const time_t *tp)
{
	if (tp == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (geteuid() != 0) {
		errno = EPERM;
		return -1;
	}
	set_real_time_clock(*tp);
	return B_OK;
}

