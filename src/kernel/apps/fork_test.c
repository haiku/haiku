/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <OS.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	pid_t child = fork();

	if (child == 0) {
		printf("CHILD: we're the child!\n");
		snooze(500000);	// .5 sec
		printf("CHILD: exit!\n");
	} else if (child > 0) {
		printf("PARENT: we're the parent, our child has pid %ld\n", child);
		waitpid(-1, NULL, 0);
	} else
		fprintf(stderr, "fork() failed: %s\n", strerror(errno));

	return 0;
}

