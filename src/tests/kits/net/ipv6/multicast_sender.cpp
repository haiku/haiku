/*
 * Copyright 2010, Atis Elsts, the.kfx@gmail.com
 * Distributed under the terms of the MIT license.
 */


#include <unistd.h>
#include <memory.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


const unsigned short TEST_PORT = 40000;


int
main(int argc, char *argv[])
{
	int socketFamily = AF_INET;

	int i = 1;
	if (argc > i) {
		if (!strcmp(argv[i], "-4")) {
			i++;
			socketFamily = AF_INET;
		} else if (!strcmp(argv[i], "-6")) {
			i++;
			socketFamily = AF_INET6;
		}
	}
	in_addr multicastSource;
	multicastSource.s_addr = 0;
	if (argc > i) {
		if (!strcmp(argv[i], "-s")) {
			++i;
			if (inet_aton(argv[i], &multicastSource) == 0) {
				fprintf(stderr, "IPv4 source address expected");
				return -1;
			}
			++i;
		}
	}

	int fd = socket(socketFamily, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	sockaddr_storage saddr;
	memset(&saddr, 0, sizeof(saddr));
	if (socketFamily == AF_INET) {
		sockaddr_in *sa = (sockaddr_in *) &saddr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(TEST_PORT);
		sa->sin_addr.s_addr = htonl(INADDR_ALLHOSTS_GROUP);
	} else {
		sockaddr_in6 *sa = (sockaddr_in6 *) &saddr;
		sa->sin6_family = AF_INET6;
		sa->sin6_port = htons(TEST_PORT);
		// ff01::1 (all nodes multicast)
		sa->sin6_addr.s6_addr[0] = 0xff;
		sa->sin6_addr.s6_addr[1] = 0x01;
		sa->sin6_addr.s6_addr[15] = 0x01;
	}

	if (multicastSource.s_addr) {
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
				&multicastSource, sizeof(multicastSource)) < 0) {
			perror("setsockopt IP_MULTICAST_IF");
			return -1;
		}
	}

	const char *buffer = "hello world";
	unsigned length = strlen(buffer);
	int status = sendto(fd, buffer, length, 0, (sockaddr *) &saddr,
		socketFamily == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
	if (status < length) {
		if (status < 0)
			perror("sendto");
		else if (status == 0)
			printf("no data sent!\n");
		else
			printf("not all data sent!\n");
	} else
		printf("send(): success\n");

	close(fd);
	return 0;
}
