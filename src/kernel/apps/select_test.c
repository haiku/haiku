/* tests basic select() and poll() functionality */

/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/select.h>
#include <poll.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FILE_NAME "/boot"

int
main(int argc, char **argv)
{
	fd_set readSet;
	struct pollfd pollfd;
	int count;
	int file = open(FILE_NAME, O_RDONLY);
	if (file < 0) {
		fprintf(stderr, "Could not open \"%s\": %s\n", FILE_NAME, strerror(file));
		return -1;
	}

	FD_ZERO(&readSet);
	FD_SET(file, &readSet);

	puts("selecting...");
	count = select(file + 1, &readSet, NULL, NULL, NULL);
	printf("\tselect returned: %d (read set = %ld)\n", count, FD_ISSET(file, &readSet));

	pollfd.fd = file;
	pollfd.events = POLLOUT | POLLERR;

	puts("polling...");
	count = poll(&pollfd, 1, -1);
	printf("\tpoll returned: %d (revents = 0x%x)\n", count, pollfd.revents);

	close(file);
	return 0;
}

