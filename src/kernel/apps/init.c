/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <unistd.h>
#include <stdio.h>
#include <syscalls.h>
#include <fcntl.h>

static void setup_io()
{
	int i;

	for(i= 0; i< 256; i++) {
		close(i);
	}

	/* XXX - open currently ignores these flags, but they
	 * should be checked once it doesn't :)
	 * Is STDERR really O_WRONLY?
	 */
	open("/dev/console", O_RDONLY, 0); /* stdin  */
	open("/dev/console", O_WRONLY, 0); /* stdout */
	open("/dev/console", O_WRONLY, 0); /* stderr */
}

int main()
{
	setup_io();

	printf("Welcome to OpenBeOS!\n");

	if(1) {
		proc_id pid;

		pid = sys_proc_create_proc("/boot/bin/fortune", "/boot/bin/fortune", NULL, 0, 5);
		if (pid >= 0) {
			int retcode;
			sys_proc_wait_on_proc(pid, &retcode);
		} else {
			printf("Failed to create a proc for fortune.\n");
		}
	}

		
	while(1) {
		proc_id pid;

		pid = sys_proc_create_proc("/boot/bin/shell", "/boot/bin/shell", NULL, 0, 5);
		if(pid >= 0) {
			int retcode;
			printf("init: spawned shell, pid 0x%x\n", pid);
			sys_proc_wait_on_proc(pid, &retcode);
			printf("init: shell exited with return code %d\n", retcode);
		} else {
			printf("Failed to start a shell :(\n");
		}
	}

	printf("init exiting\n");

	return 0;
}
