/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bruno Albuquerque, bga@bug-br.org.br
 */

#include <OS.h>

#include <sys/resource.h>
#include <sys/wait.h>


pid_t
wait3(int *status, int options, struct rusage *rusage)
{
	return wait4(-1, status, options, rusage);
}


pid_t
wait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	pid_t waitPid = waitpid(pid, status, options);
	if (waitPid != -1) {
		team_usage_info info;
		
		// Obtain info for the process that changed state.
		if (get_team_usage_info(waitPid, RUSAGE_SELF, &info) != B_OK)
			return -1;
		
		rusage->ru_utime.tv_sec = info.user_time / 1000000;
		rusage->ru_utime.tv_usec = info.user_time % 1000000;

		rusage->ru_stime.tv_sec = info.kernel_time / 1000000;
		rusage->ru_stime.tv_usec = info.kernel_time % 1000000;	
	}
	
	return waitPid;
}

