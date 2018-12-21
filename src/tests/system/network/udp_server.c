/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define MAXLEN 65536


static void
udp_server(int sockFD)
{
	char buf[MAXLEN];
	long status;

	while (1) {
		status = recvfrom(sockFD, buf, MAXLEN-1, 0, NULL, NULL);
		if (status < 0) {
			printf("recvfrom(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
		buf[status] = 0;
		printf("%s", buf);
	}
}


int
main(int argc, char** argv)
{
	long status;
	int sockFD;
	struct sockaddr_in localAddr;

	if (argc < 2) {
		printf("usage: %s <local-port>\n", argv[0]);
		exit(5);
	}

	sockFD = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&localAddr, 0, sizeof(struct sockaddr_in));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(atoi(argv[1]));
	printf("binding to port %u\n", ntohs(localAddr.sin_port));
	status = bind(sockFD, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in));
	if (status < 0) {
		printf("bind(): %lx (%s)\n", status, strerror(status));
		exit(5);
	}

	udp_server(sockFD);
	return 0;
}
