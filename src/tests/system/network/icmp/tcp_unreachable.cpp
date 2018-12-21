/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yin Qiu
 */


#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


int
main(int argc, char **argv)
{
	int sockfd, status;
	struct sockaddr_in serverAddr;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <ip-address> <port>\n", argv[0]);
		exit(1);
	}

	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ((status = connect(sockfd, (struct sockaddr*)&serverAddr,
			sizeof(struct sockaddr_in))) < 0) {
		int e = errno;
		fprintf(stderr, "Connection failed. Status: %d\n", status);
		fprintf(stderr, "Error: %s\n", strerror(e));
		exit(1);
	} else {
		printf("Connected to remote server.\n");
		close(sockfd);
		printf("Socket closed.\n");
	}
	return 0;
}
