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


#define MAXLEN 4096


int
main(int argc, char **argv)
{
	int sockfd, status;
	char *str = "Hello";
	char buf[MAXLEN];
	struct sockaddr_in serverAddr;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <ip-address> <port>\n", argv[0]);
		exit(1);
	}

	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// We need to call connect() to receive async ICMP errors
	if (connect(sockfd, (struct sockaddr*)&serverAddr,
			sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "Calling connect() on UDP socket failed\n");
		exit(1);
	}
	if ((status = write(sockfd, str, strlen(str))) < 0) {
		int e = errno;
		fprintf(stderr, "UDP send failed. Status: %d\n", status);
		fprintf(stderr, "Error: %s\n", strerror(e));
		exit(1);
	} else {
		printf("%d bytes sent to remote server.\n", status);
		if ((status = read(sockfd, buf, MAXLEN)) > 0) {
			printf("Received %d bytes from remote host\n", status);
			printf("%s\n", buf);
		} else {
			// We are expecting this
			int e = errno;
			printf("Error: %s\n", strerror(e));
		}
		close(sockfd);
		printf("Socket closed.\n");
	}
	return 0;
}
