/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <OS.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	pid_t child = fork();
	if (child == 0) {
		write(1, "CHILD: is awake!\n", 17);
		exit_thread(0);
		//printf("CHILD: we're the child!\n");
	} else if (child > 0) {
		printf("PARENT: we're the parent, our child has pid %ld\n", child);
		wait_for_thread(child, NULL);
	} else
		fprintf(stderr, "fork() failed: %s\n", strerror(errno));

	return 0;
}

