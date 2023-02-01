/*
 * Copyright 2004-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <image.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno_private.h>
#include <syscall_utils.h>


extern "C" int
system(const char *command)
{
	if (!command)
		return 1;

	const char *argv[] = { "/bin/sh", "-c", command, NULL };
	int argc = 3;

	thread_id thread = load_image(argc, argv, (const char **)environ);
	if (thread < 0)
		RETURN_AND_SET_ERRNO_TEST_CANCEL(thread);

	// block SIGCHLD ...
	sigset_t mask, oldMask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, &oldMask);

	// and ignore SIGINT and SIGQUIT while waiting for completion
	struct sigaction intSave, quitSave, sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa, &intSave);
	sigaction(SIGQUIT, &sa, &quitSave);

	resume_thread(thread);

	int exitStatus;
	pid_t result;
	while ((result = waitpid(thread, &exitStatus, 0)) < 0
		&& errno == B_INTERRUPTED) {
		// waitpid() was interrupted by a signal, retry...
	}

	// unblock and reset signal handlers
	sigprocmask(SIG_SETMASK, &oldMask, NULL);
	sigaction(SIGINT, &intSave, NULL);
	sigaction(SIGQUIT, &quitSave, NULL);

	if (result < 0)
		return -1;

	return exitStatus;
}
