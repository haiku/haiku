/*
** Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
** Distributed under the terms of the MIT License.
*/

#include <OS.h>

#include <errno.h>
#include <sys/resource.h>
#include <sys/times.h>


clock_t
times(struct tms *buffer)
{
	team_usage_info info;
	status_t err;
	int32 clk_tck = 1000000 / CLK_TCK; /* to avoid overflow */

	if ((err = get_team_usage_info(B_CURRENT_TEAM, RUSAGE_SELF, &info)) != B_OK) {
		errno = err;
		return -1;
	}

	buffer->tms_utime = info.user_time / clk_tck;
	buffer->tms_stime = info.kernel_time / clk_tck;
	
	if ((err = get_team_usage_info(B_CURRENT_TEAM, RUSAGE_CHILDREN, &info)) != B_OK) {
		errno = err;
		return -1;
	}

	buffer->tms_cutime = info.user_time / clk_tck;
	buffer->tms_cstime = info.kernel_time / clk_tck;

	return system_time() / clk_tck;
}

