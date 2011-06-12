/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.

 * Test that clock_getres() returns -1 on failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "posixtest.h"

static clockid_t get_invalid_clock_id()
{
#if _POSIX_CPUTIME == -1
	printf("clock_getcpuclockid() not supported\n");
	exit(PTS_UNTESTED);
#else
	clockid_t clockID;
	int error;

	if (sysconf(_SC_CPUTIME) == -1) {
		printf("clock_getcpuclockid() not supported\n");
		exit(PTS_UNTESTED);
	}

	int pid = fork();
	if (pid < 0) {
		perror("fork() failed");
		exit(PTS_UNRESOLVED);
	}

	if (pid == 0) {
		// child -- just wait a second
		sleep(1);
		exit(0);
	}

	// parent -- get the child's CPU clock ID
	error = clock_getcpuclockid(pid, &clockID);
	if (error != 0) {
		printf("clock_getcpuclockid() failed: %s\n", strerror(error));
		exit(PTS_UNRESOLVED);
	}

	// wait for the child
	if (wait(NULL) != pid) {
		perror("wait() failed");
		exit(PTS_UNRESOLVED);
	}

	return clockID;
#endif
}

int main(int argc, char *argv[])
{
	struct timespec res;
	clockid_t invalidClockID;

	invalidClockID = get_invalid_clock_id();

	if (clock_getres(invalidClockID, &res) == -1) {
		printf("Test PASSED\n");
		return PTS_PASS;
	} else {
		printf("Test FAILED\n");
		return PTS_FAIL;
	}

	printf("This code should not be executed\n");
	return PTS_UNRESOLVED;
}
