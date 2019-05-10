/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


extern const char *__progname;


int
main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: %s <file> <size>\n", __progname);
		return 1;
	}

	struct stat st;
	if (stat(argv[1], &st) != 0) {
		fprintf(stderr, "%s: cannot stat file \"%s\": %s\n", __progname,
			argv[1], strerror(errno));
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: could open the file read-only \"%s\": %s\n",
			__progname, argv[1], strerror(errno));
		return 1;
	}
	if (fchmod(fd, 0666) == -1) {
		fprintf(stderr, "%s: couldn't chmod a file opened read-only \"%s\": %s\n",
			__progname, argv[1], strerror(errno));
		close(fd);
		return 1;
	}
	close(fd);

	return 0;
}
