/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <syscalls.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int rc;

	if(argc < 4) {
		printf("not enough arguments to mount:\n");
		printf("usage: mount <path> <device> <fsname>\n");
		return 0;
	}

	rc = sys_mount(argv[1], argv[2], argv[3], NULL);
	if (rc < 0) {
		printf("sys_mount() returned error: %s\n", strerror(rc));
	} else {
		printf("%s successfully mounted on %s.\n", argv[2], argv[1]);
	}

	return 0;
}

