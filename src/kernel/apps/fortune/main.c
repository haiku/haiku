/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscalls.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define FORTUNES "/etc/fortunes"

int
main(void)
{
	int fd;
	int rc;
	char *buf;
	unsigned i;
	unsigned found;
	struct stat stat;

	fd = open(FORTUNES, O_RDONLY, 0);
	if (fd < 0) {
		printf("Couldn't open %s: %s\n", FORTUNES, strerror(errno));
		return -1;
	}

	rc = fstat(fd, &stat);
	if (rc < 0) {
		printf("stat() failed: %s\n", strerror(errno));
		return -1;
	}

	buf = malloc(stat.st_size + 1);
	rc = read(fd, buf, stat.st_size);
	if (rc < 0) {
		printf("Could not read from fortune file: %s\n", strerror(errno));
		return -1;
	}

	buf[stat.st_size] = 0;
	close(fd);

	found = 0;
	for (i = 0; i < stat.st_size; i++) {
		if (!strncmp(buf + i, "#@#", 3))
			found += 1;
	}

	if (found > 0)
		found = 1 + ((system_time() + 3) % found);
	else {
		printf("Out of cookies...\n");
		return -1;
	}

	for (i = 0; i < stat.st_size; i++) {
		if (!strncmp(buf + i, "#@#", 3))
			found -= 1;

		if (found == 0) {
			unsigned j;

			for (j = i + 1; j < stat.st_size; j++) {
				if (!strncmp(buf + j, "#@#", 3))
					buf[j] = 0;
			}

			printf("%s\n", buf + i + 3);
			break;
		}
	}

	return 0;
}
