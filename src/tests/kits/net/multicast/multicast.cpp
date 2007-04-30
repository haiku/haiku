/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
	char buf[256];

	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(12345);

	bind(sock, (sockaddr *)&sin, sizeof(sin));

	ip_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));

	inet_pton(AF_INET, argv[1], &mreq.imr_multiaddr);

	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

	while (1) {
		int len = recv(sock, buf, sizeof(buf), 0);
	}

	return 0;
}

