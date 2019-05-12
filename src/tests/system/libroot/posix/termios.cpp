/*
 * Copyright 2019, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


extern const char *__progname;


int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <file>\n", __progname);
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: could open the file read-only \"%s\": %s\n",
			__progname, argv[1], strerror(errno));
		return 1;
	}
	int err = tcdrain(fd);
	if (err != -1 || errno != ENOTTY) {
		fprintf(stderr, "%s: tcdrain didn't fail with ENOTTY \"%s\": %s\n",
			__progname, argv[1], strerror(errno));
		close(fd);
		return 1;
	}
	err = tcflow(fd, TCION);
	if (err != -1 || errno != ENOTTY) {
		fprintf(stderr, "%s: tcflow didn't fail with ENOTTY \"%s\": %s\n",
			__progname, argv[1], strerror(errno));
		close(fd);
		return 1;
	}

	err = tcflush(fd, TCIOFLUSH);
	if (err != -1 || errno != ENOTTY) {
		fprintf(stderr, "%s: tcflush didn't fail with ENOTTY \"%s\": %s\n",
			__progname, argv[1], strerror(errno));
		close(fd);
		return 1;
	}

	close(fd);

	return 0;
}
