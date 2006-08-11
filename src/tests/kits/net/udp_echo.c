/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define MAXLEN 65535


static void
udp_echo_client(int sockFD, const struct sockaddr_in* serverAddr)
{
	char buf[MAXLEN];
	unsigned int len;
	long status;

	while (fgets(buf, MAXLEN, stdin) != NULL) {
		len = strlen(buf);
		if (len > 0)
			len--;
printf("trying to send %u bytes...\n", len);
		status = sendto(sockFD, buf, len, 0, 
			(struct sockaddr*)serverAddr, sizeof(struct sockaddr_in));
		if (status < 0) {
			printf("sendto(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
		len = 0;
		status = recvfrom(sockFD, buf, MAXLEN-1, 0, NULL, NULL);
		if (status < 0) {
			printf("recvfrom(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
		buf[status] = 0;
		printf("-> %s\n", buf);
	}
}


static void
udp_echo_server(int sockFD)
{
	char buf[MAXLEN];
	long status;
	socklen_t len;
	long i;
	struct sockaddr_in clientAddr;

	while (1) {
		len = sizeof(clientAddr);
		status = recvfrom(sockFD, buf, MAXLEN-1, 0, (struct sockaddr*)&clientAddr, &len);
		if (status < 0) {
			printf("recvfrom(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
		buf[status] = 0;
		printf("got <%s> from client(%lx:%u)\n", buf, clientAddr.sin_addr.s_addr, clientAddr.sin_port);
		for (i = 0; i < status; ++i) {
			if (islower(buf[i]))
				buf[i] = toupper(buf[i]);
			else if (isupper(buf[i]))
				buf[i] = tolower(buf[i]);
		}
		printf("sending <%s>\n", buf);
		status = sendto(sockFD, buf, status, 0, 
			(struct sockaddr*)&clientAddr, sizeof(clientAddr));
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
	struct sockaddr_in serverAddr, localAddr;
	int clientMode;
	unsigned short bindPort = 0;

	if (argc < 2) {
		printf("usage: %s client <IP-address> <port> [local-port]\n", argv[0]);
		printf("or     %s server <local-port>\n", argv[0]);
		exit(5);
	}

	if (!strcmp(argv[1], "client")) {
		clientMode = 1;
		if (argc < 4) {
			printf("usage: %s client <IP-address> <port> [local-port]\n", argv[0]);
			exit(5);
		}
		memset(&serverAddr, 0, sizeof(struct sockaddr_in));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(atoi(argv[3]));
		serverAddr.sin_addr.s_addr = inet_addr(argv[2]);
		if (argc > 4)
			bindPort = atoi(argv[4]);
		printf("client connected to server(%lx:%u)\n", serverAddr.sin_addr.s_addr, 
			ntohs(serverAddr.sin_port));
	} else if (!strcmp(argv[1], "server")) {
		clientMode = 0;
		if (argc < 3) {
			printf("usage: %s server <local-port>\n", argv[0]);
			exit(5);
		}
		bindPort = atoi(argv[2]);
	}

	sockFD = socket(AF_INET, SOCK_DGRAM, 0);

	if (bindPort > 0) {
		memset(&localAddr, 0, sizeof(struct sockaddr_in));
		localAddr.sin_family = AF_INET;
		localAddr.sin_port = htons(bindPort);
		printf("binding to port %u\n", bindPort);
		status = bind(sockFD, (struct sockaddr *)&localAddr, sizeof(localAddr));
		if (status < 0) {
			printf("bind(): %lx (%s)\n", status, strerror(status));
			exit(5);
		}
	}

	if (clientMode)
		udp_echo_client(sockFD, &serverAddr);
	else
		udp_echo_server(sockFD);

	return 0;
}
