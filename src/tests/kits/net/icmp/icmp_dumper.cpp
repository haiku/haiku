/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yin Qiu
 */


#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define MAXLEN 4096


int
main(void)
{
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		fprintf(stderr, "Could not open raw socket: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_in source;
	socklen_t addrLen = sizeof(source);
	char buf[MAXLEN];
	ssize_t nbytes;

	while ((nbytes = recvfrom(sockfd, buf, MAXLEN, 0,
			(struct sockaddr*)&source, &addrLen)) > 0) {
		int ipLen, icmpLen;

		char host[128];
		if (!inet_ntop(AF_INET, &source.sin_addr, host, sizeof(host)))
			strcpy(host, "<unknown host>");

		printf("Received %zd bytes of ICMP message from %s\n", nbytes, host);

		struct ip* ip = (struct ip*)buf;
		ipLen = ip->ip_hl << 2;
		if ((icmpLen = nbytes - ipLen) < 8) {
			fprintf(stderr, "ICMP len (%d) < 8\n", icmpLen);
			exit(1);
		}
		struct icmp* icmp = (struct icmp*)(buf + ipLen);
		printf("Type: %u; Code: %u\n", icmp->icmp_type, icmp->icmp_code);
	}

	return 0;
}
