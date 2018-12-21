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

#define MAXLEN 32768


static void
udp_client(int sockFD, const struct sockaddr_in* serverAddr)
{
	char sendbuf[MAXLEN];
	long status;

	while (!feof(stdin)) {
		status = fread(sendbuf, 1, MAXLEN, stdin);
		if (status < 0) {
			printf("fread(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
printf("trying to send %ld bytes...\n", status);
		status = sendto(sockFD, sendbuf, status, 0, 
			(struct sockaddr*)serverAddr, sizeof(struct sockaddr_in));
		if (status < 0) {
			printf("sendto(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
	}
}


int
main(int argc, char** argv)
{
	long status;
	int sockFD;
	struct sockaddr_in serverAddr, clientAddr;

	if (argc < 3) {
		printf("usage: %s <IP-address> <port> [local-port]\n", argv[0]);
		exit(5);
	}

	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
	printf("addr=%lx port=%u\n", serverAddr.sin_addr.s_addr, ntohs(serverAddr.sin_port));

	sockFD = socket(AF_INET, SOCK_DGRAM, 0);

	if (argc > 3) {
		memset(&clientAddr, 0, sizeof(struct sockaddr_in));
		clientAddr.sin_family = AF_INET;
		clientAddr.sin_port = htons(atoi(argv[3]));
		printf("binding to port %u\n", ntohs(clientAddr.sin_port));
		status = bind(sockFD, (struct sockaddr *)&clientAddr, sizeof(struct sockaddr_in));
		if (status < 0) {
			printf("bind(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
	}

	udp_client(sockFD, &serverAddr);
	return 0;
}
