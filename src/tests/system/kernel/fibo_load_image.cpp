/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <image.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


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
		char buffer[64];
		char* args[]= { argv[0], "-s", buffer, NULL };
		int argCount = 3;
		status_t status;

		snprintf(buffer, sizeof(buffer), "%d", num - 1);
		thread_id threadA = load_image(argCount, (const char**)args, (const char**)environ);
		if (threadA < B_OK) {
			fprintf(stderr, "Could not create thread A: %s\n", strerror(threadA));
			return -1;
		}

		snprintf(buffer, sizeof(buffer), "%d", num - 2);
		thread_id threadB = load_image(argCount, (const char**)args, (const char**)environ);
		if (threadB < B_OK) {
			fprintf(stderr, "Could not create thread B: %s\n", strerror(threadB));
			return -1;
		}

		resume_thread(threadA);
		resume_thread(threadB);

		status_t returnValue = 0;
		do {
			status = wait_for_thread(threadA, &returnValue);
		} while (status == B_INTERRUPTED);

		if (status == B_OK)
			result = returnValue;
		else
			fprintf(stderr, "wait_for_thread(%ld) A failed: %s\n", threadA, strerror(status));

		do {
			status = wait_for_thread(threadB, &returnValue);
		} while (status == B_INTERRUPTED);

		if (status == B_OK)
			result += returnValue;
		else
			fprintf(stderr, "wait_for_thread(%ld) B failed: %s\n", threadB, strerror(status));
	}

	if (silent) {
		return result;
	} else {
		printf("%d\n", result);
		return 0;
	}
}

