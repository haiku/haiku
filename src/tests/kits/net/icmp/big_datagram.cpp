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
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>


#define DGRM_SIZE 1600 // ought to be large enough


struct pseudo_header {
	struct in_addr src;
	struct in_addr dst;	
	u_int8_t zero;
	u_int8_t protocol;
	u_int16_t len;
};


static u_int16_t
cksum(u_int16_t* buf, int nwords)
{
	u_int32_t sum;
	for (sum = 0; nwords > 0; nwords--)
		sum += *(buf++);
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}


static struct addrinfo*
host_serv(const char* host, const char* serv, int family, int socktype)
{
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = family;
	hints.ai_socktype = socktype;

	int n = getaddrinfo(host, serv, &hints, &res);
	if (n != 0)
		return NULL;

	return res;
}


int
main(int argc, char** argv)
{
	if (argc < 3) {
		printf("Usage: %s <ip-address> <port> [nbytes]\n", argv[0]);
		return 1;
	}

	size_t size;
	if (argc == 3)
		size = DGRM_SIZE;
	else
		size = atoi(argv[3]);

	if (size - sizeof(struct ip) - sizeof(struct udphdr) < 0) {
		fprintf(stderr, "Datagram size is too small\n");
		return 1;
	}

	struct addrinfo* ai = host_serv(argv[1], NULL, 0, 0);
	if (ai == NULL) {
		fprintf(stderr, "Cannot resolve %s\n", argv[1]);
		return 1;
	}

	printf("Using datagram size %zu\n", size);

	char* datagram = (char*)malloc(size);
	if (datagram == NULL) {
		fprintf(stderr, "Out of memory.\n");
		return 1;
	}

	memset(datagram, 0, size);

	struct ip* iph = (struct ip*)datagram;
	iph->ip_hl = 4;
	iph->ip_v = 4;
	iph->ip_tos = 0;
	iph->ip_len = htons(size);
	iph->ip_id = htonl(54321);
	iph->ip_off = 0;
	iph->ip_ttl = 255;
	iph->ip_p = IPPROTO_ICMP;
	iph->ip_sum = 0;
	iph->ip_src.s_addr = INADDR_ANY;
	iph->ip_dst.s_addr = ((struct sockaddr_in*)ai->ai_addr)->sin_addr.s_addr;
	// Calculates IP checksum
	iph->ip_sum = cksum((unsigned short*)datagram, iph->ip_len >> 1);

	struct pseudo_header pHeader; // used to calculate udp checksum

	struct udphdr* udp = (struct udphdr*)(datagram + sizeof(struct ip));
	udp->uh_sport = 0;
	udp->uh_dport = htons(atoi(argv[2]));
	udp->uh_ulen = htons(size - sizeof(struct ip));
	pHeader.src = iph->ip_src;
	pHeader.dst = iph->ip_dst;
	pHeader.zero = 0;
	pHeader.protocol = iph->ip_p;
	pHeader.len = udp->uh_ulen;
	udp->uh_sum = cksum((u_int16_t*)&pHeader, 3);
	
	// Send it via a raw socket
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (sockfd < 0) {
		fprintf(stderr, "Failed to create raw socket: %s\n", strerror(errno));
		free(datagram);
		return 1;
	}

	int one = 1;
	const int* val = &one;
	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof(int)) != 0) {
		fprintf(stderr, "Failed to set IP_HDRINCL on socket: %s\n",
			strerror(errno));
		free(datagram);
		return 1;
	}

	int bytesSent = sendto(sockfd, datagram, size, 0, ai->ai_addr,
		ai->ai_addrlen);
	if (bytesSent < 0) {
		fprintf(stderr, "Failed to send the datagram via the raw socket: %s\n",
			strerror(errno));
		free(datagram);
		return 1;
	}

	printf("Sent %d bytes.\n", bytesSent);
	return 0;
}

