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


static void
usage(char const *app)
{
	printf("usage: %s [-s] ###\n", app);
	exit(-1);
}


int
main(int argc, char *argv[])
{
	bool silent = false;
	int num = 0;
	int result;

	switch (argc) {
		case 2:
			num = atoi(argv[1]);
			break;
		case 3:
			if (!strcmp(argv[1], "-s")) {
				num = atoi(argv[2]);
				silent = true;
				break;
			}
			// supposed to fall through

		default:
			usage(argv[0]);
			break;
	}

	if (num < 2) {
		result = num;
	} else {
		pid_t childA = fork();
		if (childA == 0) {
			// we're the child
			char buffer[64];
			char* args[]= { argv[0], "-s", buffer, NULL };

			snprintf(buffer, sizeof(buffer), "%d", num - 1);
			if (execv(args[0], args) < 0) {
				fprintf(stderr, "Could not create exec A: %s\n", strerror(errno));
				return -1;
			}
		} else if (childA < 0) {
			fprintf(stderr, "fork() failed for child A: %s\n", strerror(errno));
			return -1;
		}

		pid_t childB = fork();
		if (childB == 0) {
			// we're the child
			char buffer[64];
			char* args[]= { argv[0], "-s", buffer, NULL };

			snprintf(buffer, sizeof(buffer), "%d", num - 2);
			if (execv(args[0], args) < 0) {
				fprintf(stderr, "Could not create exec B: %s\n", strerror(errno));
				return -1;
			}
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

	if (silent) {
		return result;
	} else {
		printf("%d\n", result);
		return 0;
	}
}

