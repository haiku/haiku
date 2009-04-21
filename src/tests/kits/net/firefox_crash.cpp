/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

//!	A test app trying to reproduce bug #2197.

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>


static int
open_tcp_socket()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	// make it non-blocking
	fcntl(fd, F_SETFL, O_NONBLOCK);

	return fd;
}


static void
init_sockaddr(struct sockaddr_in& address, unsigned ipAddress, unsigned port)
{
	memset(&address, 0, sizeof(sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(ipAddress);
}


static int
tcp_pair(int pair[])
{
	pair[0] = pair[1] = -1;

	int listenSocket = open_tcp_socket();
	if (listenSocket < 0)
		return -1;

	sockaddr_in listenAddress;
	sockaddr_in peerAddress;
	sockaddr_in address;
	socklen_t length;
	init_sockaddr(listenAddress, INADDR_LOOPBACK, 0);

	if (bind(listenSocket, (sockaddr*)&listenAddress, sizeof(sockaddr_in)) != 0)
		goto error;

	length = sizeof(sockaddr_in);
	if (getsockname(listenSocket, (sockaddr*)&listenAddress, &length) != 0)
		goto error;

	if (listen(listenSocket, 5) != 0)
		goto error;

	pair[0] = open_tcp_socket();
	if (pair[0] < 0)
		goto error;

	init_sockaddr(address, INADDR_LOOPBACK, ntohs(listenAddress.sin_port));

	if (connect(pair[0], (sockaddr*)&address, sizeof(sockaddr_in)) != 0
		&& errno != EINPROGRESS)
		goto error;

	if (errno == EINPROGRESS) {
		struct timeval tv;
		tv.tv_sec = 100000;
		tv.tv_usec = 0;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(pair[0], &set);
		if (select(pair[0] + 1, NULL, &set, NULL, &tv) < 0)
			fprintf(stderr, "write select() failed: %s\n", strerror(errno));
	}

	length = sizeof(sockaddr_in);
	if (getsockname(pair[0], (sockaddr*)&address, &length) != 0)
		goto error;

	while (true) {
		pair[1] = accept(listenSocket, (sockaddr*)&peerAddress, &length);
		if (pair[1] >= 0)
			break;

		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != ETIMEDOUT)
			goto error;

		struct timeval tv;
		tv.tv_sec = 100000;
		tv.tv_usec = 0;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(listenSocket, &set);
		if (select(listenSocket + 1, &set, NULL, NULL, &tv) < 0)
			fprintf(stderr, "read select() failed: %s\n", strerror(errno));
	}

	if (peerAddress.sin_port != address.sin_port)
		goto error;

	close(listenSocket);
    return 0;

error:
	close(listenSocket);
	for (int i = 0; i < 2; i++) {
		if (pair[i] >= 0)
			close(pair[i]);
	}

    return -1;
}


int
main(int argc, char** argv)
{
	int pair[2];
	if (tcp_pair(pair) == 0) {
		close(pair[0]);
		close(pair[1]);
	} else
		fprintf(stderr, "pair failed: %s\n", strerror(errno));
	return 0;
}

