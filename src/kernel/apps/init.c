/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <unistd.h>
#include <stdio.h>
#include <syscalls.h>
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
main()
{
	setup_io();

	printf("Welcome to OpenBeOS!\n");

	{
		team_id pid;

		pid = _kern_create_team("/bin/fortune", "/bin/fortune", NULL, 0, NULL, 0, 5);
		if (pid >= 0) {
			int retcode;
			_kern_wait_for_team(pid, &retcode);
		} else
			printf("Failed to create a team for fortune.\n");
	}

	while (1) {
		team_id pid;

		pid = _kern_create_team("/bin/shell", "/bin/shell", NULL, 0, NULL, 0, 5);
		if (pid >= 0) {
			int retcode;
			printf("init: spawned shell, pid 0x%lx\n", pid);
			_kern_wait_for_team(pid, &retcode);
			printf("init: shell exited with return code %d\n", retcode);
		} else
			printf("Failed to start a shell :(\n");
	}

	printf("init exiting\n");

	return 0;
}
