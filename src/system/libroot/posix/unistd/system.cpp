/*
 * Copyright 2004-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <image.h>

#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


extern "C" int
system(const char *command)
{
	if (!command)
		return 1;

	const char *argv[] = { "/bin/sh", "-c", command, NULL };
	int argc = 3;

	thread_id thread = load_image(argc, argv, (const char **)environ);
	if (thread < 0) {
		errno = thread;
		return -1;
	}

	resume_thread(thread);

	int exitStatus;
	pid_t result;
	while ((result = waitpid(thread, &exitStatus, 0)) < 0
		&& errno == B_INTERRUPTED) {
		// waitpid() was interrupted by a signal, retry...
	}

	if (result < 0)
		return -1;

	return exitStatus;
}
