/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


static bool sReceiveMode = false;


int
main(int argc, char** argv)
{
	const char* peerAddress = "192.168.0.1";
	if (argc > 1)
		peerAddress = argv[1];

	if (argc > 2 && !strcmp(argv[2], "recv"))
		sReceiveMode = true;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return 1;

#if 1
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(sReceiveMode ? 8888 : 0);
	if (bind(fd, (sockaddr*)&addr, sizeof(addr)) != 0)
		perror("bind");

	sockaddr ours;
	socklen_t ourLength = sizeof(sockaddr);
	if (getsockname(fd, &ours, &ourLength) != 0)
		perror("getsockname");
	else {
		printf("After bind: local address %s (%u)\n",
			inet_ntoa(((sockaddr_in&)ours).sin_addr),
			ntohs(((sockaddr_in&)ours).sin_port));
	}
#endif

	if (!sReceiveMode) {
		sockaddr_in peer;
		peer.sin_family = AF_INET;
		peer.sin_addr.s_addr = inet_addr(peerAddress);
		peer.sin_port = htons(sReceiveMode ? 0 : 8888);
		if (connect(fd, (sockaddr*)&peer, sizeof(peer)) != 0)
			perror("connect");

		ourLength = sizeof(sockaddr);
		if (getsockname(fd, &ours, &ourLength) != 0)
			perror("getsockname");
		else {
			printf("After connect: local address %s (%u)\n",
				inet_ntoa(((sockaddr_in&)ours).sin_addr),
				ntohs(((sockaddr_in&)ours).sin_port));
		}
	}

	char buffer[256];
	if (sReceiveMode) {
		ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0) {
			buffer[bytesReceived] = 0;
			printf("received: %s\n", buffer);
		} else
			perror("recv");
	} else {
		if (argc > 2)
			send(fd, argv[2], strlen(argv[2]), 0);
		else {
			fgets(buffer, sizeof(buffer), stdin);
			send(fd, buffer, strlen(buffer), 0);
		}
	}

	close(fd);
	return 0;
}


