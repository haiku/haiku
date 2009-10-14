/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static status_t
abort_thread(void*)
{
	snooze(50000);
	abort();
	return 0;
}


int
main(int argc, char** argv)
{
	thread_id thread = spawn_thread(&abort_thread, "abort test",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(thread);

	status_t status = wait_for_thread(thread, NULL);
	fprintf(stderr, "abort thread aborted: %s\n", strerror(status));

	snooze(1000000LL);
	fprintf(stderr, "main exiting\n");
	return 0;
}
