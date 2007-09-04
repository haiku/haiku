/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


static void
restore_old_sighup(int result, struct sigaction *action)
{
	if (result != -1)
		sigaction(SIGHUP, action, NULL);
}


int
daemon(int noChangeDir, int noClose)
{
	struct sigaction oldAction, action;
	int oldActionResult;
	pid_t newGroup;
	pid_t pid;

	/* Ignore eventually send SIGHUPS on parent exit */
	sigemptyset(&action.sa_mask);
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	oldActionResult = sigaction(SIGHUP, &action, &oldAction);

	pid = fork();
	if (pid == -1) {
		restore_old_sighup(oldActionResult, &oldAction);
		return -1;
	}
	if (pid > 0) {
		// we're the parent - let's exit
		exit(0);
	}

	newGroup = setsid();
	restore_old_sighup(oldActionResult, &oldAction);

	if (newGroup == -1)
		return -1;

	if (!noChangeDir)
		chdir("/");

	if (!noClose) {
		int fd = open("/dev/null", O_RDWR);
		if (fd != -1) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			if (fd > STDERR_FILENO)
				close(fd);
		}
	}

	return 0;
}
