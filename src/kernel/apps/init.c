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

	open("/dev/keyboard", O_RDONLY, 0); /* stdin  */
	open("/dev/console", O_WRONLY, 0); /* stdout */
	open("/dev/console", O_WRONLY, 0); /* stderr */
}


int
main(int argc, char **argv)
{
	setup_io();

	printf("Welcome to Haiku!\r\n");

	{
		const char *args[] = {"consoled", NULL};
		thread_id thread;

		thread = load_image(1, args, (const char **)environ);
		if (thread >= B_OK) {
			status_t returnCode;
			wait_for_thread(thread, &returnCode);
		} else
			printf("Failed to create a team for fortune.\n");
	}

	printf("init exiting\n");

	return 0;
}
