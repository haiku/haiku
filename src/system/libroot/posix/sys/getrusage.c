/*
** Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <OS.h>
#include <sys/resource.h>
#include <errno.h>

#include <errno_private.h>


int
getrusage(int who, struct rusage *rusage)
{
	team_usage_info info;

	if (get_team_usage_info(B_CURRENT_TEAM, who, &info) != B_OK) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	rusage->ru_utime.tv_sec = info.user_time / 1000000;
	rusage->ru_utime.tv_usec = info.user_time % 1000000;

	rusage->ru_stime.tv_sec = info.kernel_time / 1000000;
	rusage->ru_stime.tv_usec = info.kernel_time % 1000000;

	return 0;
}

