/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h> 
#include <sys/wait.h>
#include <unistd.h>


/*!
	wait() should wait only once. If any argument is given, waitpid() should return
	an error (and errno to ECHILD), since there is no child with that process group ID.
*/


int
child2()
{
	sleep(2);
	return 2;
}


//! exits before child 2
int
child1()
{
	setpgrp();
		// put us into a new process group

	pid_t child = fork();
	if (child == 0)
		return child2();

	sleep(1);
	return 1;
}


int
main(int argc, char** argv)
{
	bool waitForGroup = argc > 1;

	pid_t child = fork();
	if (child == 0)
		return child1();

	struct rusage usage;
	pid_t pid;
	do {
		memset(&usage, 0, sizeof(usage));
		int childStatus = -1;
		pid = wait4(-1, &childStatus, 0, &usage);
		printf("wait4() returned %" PRId32 " (%s), child status %" PRId32
			", kernel: %ld.%06" PRId32 " user: %ld.%06" PRId32 "\n",
			pid, strerror(errno), childStatus, usage.ru_stime.tv_sec,
			usage.ru_stime.tv_usec, usage.ru_utime.tv_sec,
			usage.ru_utime.tv_usec);
	} while (pid >= 0);

	return 0;
}

