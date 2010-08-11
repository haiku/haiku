/*
 * Copyright 2010, Atis Elsts, the.kfx@gmail.com
 * Distributed under the terms of the MIT license.
 */


#include <unistd.h>
#include <memory.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


const unsigned short TEST_PROTOCOL = 89;


void
recvLoop(int fd)
{
	for (;;) {
		char buffer[1000];
		sockaddr_storage address;
		socklen_t socklen = sizeof(address);
		memset(&address, 0, socklen);

		int status = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
			(sockaddr *) &address, &socklen);
		if (status < 0) {
			perror("recvfrom");
			exit(-1);
		}
		if (status == 0) {
			printf("received EOF!\n");
			break;
		} else {
			char *p = buffer;
			if (address.ss_family == AF_INET && status >= sizeof(struct ip))
				p += sizeof(struct ip);
			buffer[status] = 0;
			printf("received %d bytes: \"%s\"\n", status, p);
		}
	}
}


int
main(int argc, char *argv[])
{
	int socketFamily = AF_INET;

	if (argc > 1) {
		if (!strcmp(argv[1], "-4"))
			socketFamily = AF_INET;
		else if (!strcmp(argv[1], "-6"))
			socketFamily = AF_INET6;
	}

	int fd = socket(socketFamily, SOCK_RAW, TEST_PROTOCOL);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	sockaddr_storage localAddress;
	memset(&localAddress, 0, sizeof(localAddress));
	if (socketFamily == AF_INET) {
		sockaddr_in *sa = (sockaddr_in *) &localAddress;
		sa->sin_family = AF_INET;
	} else {
		sockaddr_in6 *sa = (sockaddr_in6 *) &localAddress;
		sa->sin6_family = AF_INET6;
	}

	// TODO: is this even necessary?
	if (bind(fd, (sockaddr *)&localAddress, socketFamily == AF_INET ?
			sizeof(sockaddr_in) : sizeof(sockaddr_in6)) < 0) {
		perror("bind");
		return -1;
	}

	for (;;)
		recvLoop(fd);
}
