/* tests node monitor functionality (very basic test!) */

/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <NodeMonitor.h>
#include <syscalls.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>


int
main(int argc, char **argv)
{
	struct stat st;
	if (stat("/", &st) < 0) {
		fprintf(stderr, "Could not stat root!\n");
		return -1;
	}

	printf("watch file: device = %ld, node = %Ld\n", st.st_dev, st.st_ino);

	if (sys_start_watching(st.st_dev, st.st_ino, B_WATCH_DIRECTORY, 1, 2) < B_OK) {
		fprintf(stderr, "Could not start watching!\n");
		return -1;
	}

	mkdir("/temp_test", 0755);
	rmdir("/temp_test");

	if (sys_stop_watching(st.st_dev, st.st_ino, 0, 1, 2) < B_OK) {
		fprintf(stderr, "Could not stop watching!\n");
		return -1;
	}

	return 0;
}

