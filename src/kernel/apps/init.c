/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <image.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


static void
setup_io(void)
{
	int i;

	for (i = 0; i< 256; i++) {
		close(i);
	}

	open("/dev/console", O_RDONLY, 0); /* stdin  */
	open("/dev/console", O_WRONLY, 0); /* stdout */
	open("/dev/console", O_WRONLY, 0); /* stderr */
}


int
main(int argc, char **argv)
{
	setup_io();

	printf("Welcome to Haiku!\n");

	{
		const char *args[] = {"fortune", NULL};
		thread_id thread;

		thread = load_image(1, args, (const char **)environ);
		if (thread >= B_OK) {
			status_t returnCode;
			wait_for_thread(thread, &returnCode);
		} else
			printf("Failed to create a team for fortune.\n");
	}

	while (1) {
		const char *args[] = {"shell", NULL};
		thread_id thread;

		thread = load_image(1, args, (const char **)environ);
		if (thread >= B_OK) {
			status_t returnCode;

			printf("init: spawned shell, pid 0x%lx\n", thread);
			wait_for_thread(thread, &returnCode);
			printf("init: shell exited with return code %ld\n", returnCode);
		} else
			printf("Failed to start a shell :(\n");
	}

	printf("init exiting\n");

	return 0;
}
