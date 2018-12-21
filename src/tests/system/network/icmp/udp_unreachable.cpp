/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yin Qiu
 */


#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define MAX_LENGTH 4096


extern const char* __progname;


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <host> [<port>]\n"
			"Sends a UDP datagram to the specified target.\n"
			"<port> defaults to 9999.\n", __progname);
		return 1;
	}

	struct hostent* host = gethostbyname(argv[1]);
	if (host == NULL) {
		perror("gethostbyname");
		return 1;
	}

	uint16_t port = 9999;
	if (argc > 2)
		port = atol(argv[2]);

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr = *((struct in_addr*)host->h_addr);

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// We need to call connect() to receive async ICMP errors
	if (connect(sockfd, (struct sockaddr*)&serverAddr,
			sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "Calling connect() on UDP socket failed: %s\n",
			strerror(errno));
		return 1;
	}

	const char* string = "Hello";
	ssize_t bytes = write(sockfd, string, strlen(string));
	if (bytes < 0) {
		fprintf(stderr, "UDP send failed: %s\n", strerror(errno));
		return 1;
	}

	printf("%zd bytes sent to remote server.\n", bytes);

	char buffer[MAX_LENGTH];
	bytes = read(sockfd, buffer, MAX_LENGTH);
	if (bytes > 0) {
		printf("Received %zd bytes from remote host\n", bytes);
		printf("%s\n", buffer);
	} else {
		// We are expecting this
		printf("Error: %s\n", strerror(errno));
	}
	close(sockfd);
	printf("Socket closed.\n");

	return 0;
}
