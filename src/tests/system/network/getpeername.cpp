/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


extern const char* __progname;


int
main(int argc, char** argv)
{
	if (argc < 2) {
		printf("usage: %s <host> [port]\n"
			"Connects to the host (default port 21, ftp), and calls\n"
			"getpeername() to see if it works correctly with a short\n"
			"buffer.\n", __progname);
		return 0;
	}

	struct hostent* host = gethostbyname(argv[1]);
	if (host == NULL) {
		perror("gethostbyname");
		return 1;
	}

	uint16_t port = 21;
	if (argc > 2)
		port = atol(argv[2]);

	int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0) {
		perror("socket");
		return 1;
	}

	sockaddr_in address;
	memset(&address, 0, sizeof(sockaddr_in));

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr = *((struct in_addr*)host->h_addr);

	socklen_t length = 14;
	sockaddr buffer;
	int result = getpeername(socket, &buffer, &length);
	printf("getpeername() before connect(): %d (%s), length: %d\n", result,
		strerror(errno), length);

	if (connect(socket, (sockaddr*)&address, sizeof(sockaddr_in)) < 0) {
		perror("connect");
		return 1;
	}

	errno = 0;
	length = 14;
	result = getpeername(socket, &buffer, &length);
	printf("getpeername() after connect(): %d (%s), length: %d\n", result,
		strerror(errno), length);

	close(socket);
	return 0;
}

