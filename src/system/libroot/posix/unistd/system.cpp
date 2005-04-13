/* 
** Copyright 2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <image.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


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

	status_t returnValue;
	status_t error = wait_for_thread(thread, &returnValue);
	if (error != B_OK) {
		errno = error;
		return -1;
	}
	return returnValue;
}
