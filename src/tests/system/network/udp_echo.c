/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define MAXLEN 65535


#ifndef HAIKU_TARGET_PLATFORM_HAIKU
typedef int socklen_t;
#endif

extern const char* __progname;


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
			printf("sendto(): %x (%s)\n", errno, strerror(errno));
			exit(5);
		}
		len = 0;
		status = recvfrom(sockFD, buf, MAXLEN-1, 0, NULL, NULL);
		if (status < 0) {
			printf("recvfrom(): %x (%s)\n", errno, strerror(errno));
			exit(5);
		}
		buf[status] = 0;
		printf("-> %s\n", buf);
	}
}


static void
udp_broadcast(int sockFD, const struct sockaddr_in* serverAddr)
{
	char buf[MAXLEN];
	int option = 1;
	int len;
	int status;

	strcpy(buf, "broadcast");
	len = strlen(buf);

	setsockopt(sockFD, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option));

	status = sendto(sockFD, buf, len, 0,
		(struct sockaddr*)serverAddr, sizeof(struct sockaddr_in));
	if (status < 0) {
		printf("sendto(): %s\n", strerror(errno));
		exit(5);
	}

	status = recvfrom(sockFD, buf, MAXLEN-1, 0, NULL, NULL);
	if (status < 0) {
		printf("recvfrom(): %x (%s)\n", errno, strerror(errno));
		exit(5);
	}
	buf[status] = 0;
	printf("-> %s\n", buf);
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
			printf("recvfrom(): %x (%s)\n", errno, strerror(errno));
			exit(5);
		}
		buf[status] = 0;
		printf("got <%s> from client(%08x:%u)\n", buf, clientAddr.sin_addr.s_addr, clientAddr.sin_port);
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
			printf("sendto(): %x (%s)\n", errno, strerror(errno));
			exit(5);
		}
	}
}


int
main(int argc, char** argv)
{
	unsigned long status;
	int sockFD;
	struct sockaddr_in serverAddr, localAddr;
	enum {
		CLIENT_MODE,
		BROADCAST_MODE,
		SERVER_MODE,
	} mode = 0;
	unsigned short bindPort = 0;
	const char* bindAddr = NULL;

	if (argc < 2) {
		printf("usage: %s client <IP-address> <port> [local-port]\n", __progname);
		printf("or     %s broadcast <port> <local-port>\n", __progname);
		printf("or     %s server <local-port>\n", __progname);
		exit(5);
	}

	if (!strcmp(argv[1], "client")) {
		mode = CLIENT_MODE;
		if (argc < 4) {
			printf("usage: %s client <IP-address> <port> [local-port]\n", __progname);
			exit(5);
		}
		memset(&serverAddr, 0, sizeof(struct sockaddr_in));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(atoi(argv[3]));
		serverAddr.sin_addr.s_addr = inet_addr(argv[2]);
		if (argc > 4)
			bindPort = atoi(argv[4]);
		printf("client connected to server(%08x:%u)\n", serverAddr.sin_addr.s_addr,
			ntohs(serverAddr.sin_port));
	} else if (!strcmp(argv[1], "broadcast")) {
		mode = BROADCAST_MODE;
		if (argc < 3) {
			printf("usage: %s broadcast <port> [local-addr] [broadcast-addr] [local-port]\n", __progname);
			exit(5);
		}

		if (argc > 3)
			bindAddr = argv[3];

		memset(&serverAddr, 0, sizeof(struct sockaddr_in));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(atoi(argv[2]));
		if (argc > 4)
			serverAddr.sin_addr.s_addr = inet_addr(argv[4]);
		else
			serverAddr.sin_addr.s_addr = INADDR_BROADCAST;

		if (argc > 5)
			bindPort = atoi(argv[5]);
	} else if (!strcmp(argv[1], "server")) {
		mode = SERVER_MODE;
		if (argc < 3) {
			printf("usage: %s server <local-port>\n", argv[0]);
			exit(5);
		}
		bindPort = atoi(argv[2]);
	}

	sockFD = socket(AF_INET, SOCK_DGRAM, 0);

	if (bindAddr != NULL || bindPort > 0) {
		memset(&localAddr, 0, sizeof(struct sockaddr_in));
		localAddr.sin_family = AF_INET;
		if (bindAddr != NULL) {
			localAddr.sin_addr.s_addr = inet_addr(bindAddr);
			printf("binding to addr %s\n", bindAddr);
		}
		if (bindPort > 0) {
			localAddr.sin_port = htons(bindPort);
			printf("binding to port %u\n", bindPort);
		}
		status = bind(sockFD, (struct sockaddr *)&localAddr, sizeof(localAddr));
		if (status < 0) {
			printf("bind(): %x (%s)\n", errno, strerror(errno));
			exit(5);
		}
	}

	switch (mode) {
		case CLIENT_MODE:
			udp_echo_client(sockFD, &serverAddr);
			break;
		case BROADCAST_MODE:
			udp_broadcast(sockFD, &serverAddr);
			break;
		case SERVER_MODE:
			udp_echo_server(sockFD);
			break;
	}

	return 0;
}
