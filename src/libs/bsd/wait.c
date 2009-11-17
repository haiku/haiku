/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bruno Albuquerque, bga@bug-br.org.br
 */


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
	if (waitPid != -1)
		getrusage(RUSAGE_CHILDREN, rusage);

	return waitPid;
}

