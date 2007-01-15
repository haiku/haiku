/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <image.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int gForked = 0;


static void
usage(char const *app)
{
	printf("usage: %s ###\n", app);
	exit(-1);
}


int
main(int argc, char *argv[])
{
	int result = 0;

	if (argc != 2)
		usage(argv[0]);

	int num = atoi(argv[1]);

	if (num < 2) {
		result = num;
	} else {
		pid_t childA = fork();
		if (childA == 0) {
			// we're the child
			char buffer[64];
			char* args[]= { argv[0], buffer, NULL };
			int argCount = 2;
			gForked++;

			snprintf(buffer, sizeof(buffer), "%d", num - 1);
			return main(argCount, args);
		} else if (childA < 0) {
			fprintf(stderr, "fork() failed for child A: %s\n", strerror(errno));
			return -1;
		}

		pid_t childB = fork();
		if (childB == 0) {
			// we're the child
			char buffer[64];
			char* args[]= { argv[0], buffer, NULL };
			int argCount = 2;
			gForked++;

			snprintf(buffer, sizeof(buffer), "%d", num - 2);
			return main(argCount, args);
		} else if (childB < 0) {
			fprintf(stderr, "fork() failed for child B: %s\n", strerror(errno));
			return -1;
		}

		status_t status, returnValue = 0;
		do {
			status = wait_for_thread(childA, &returnValue);
		} while (status == B_INTERRUPTED);

		if (status == B_OK)
			result = returnValue;
		else
			fprintf(stderr, "wait_for_thread(%ld) A failed: %s\n", childA, strerror(status));

		do {
			status = wait_for_thread(childB, &returnValue);
		} while (status == B_INTERRUPTED);

		if (status == B_OK)
			result += returnValue;
		else
			fprintf(stderr, "wait_for_thread(%ld) B failed: %s\n", childB, strerror(status));
	}

	if (gForked) {
		return result;
	} else {
		printf("%d\n", result);
		return 0;
	}
}

