/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h> 
#define _BSD_SOURCE
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
		int childStatus = -1;
		pid = wait4(-1, &childStatus, 0, &usage);
		printf("wait4() returned %ld (%s), child status %d\n",
			pid, strerror(errno), childStatus);
	} while (pid >= 0);

	return 0;
}

