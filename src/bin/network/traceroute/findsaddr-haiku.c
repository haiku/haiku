/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hugo Santos <hugosantos@gmail.com>
 */


#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include "findsaddr.h"
	// is not self containing...


const char *
findsaddr(const struct sockaddr_in *to, struct sockaddr_in *from)
{
	uint8_t buffer[256];
	struct route_entry *request = (struct route_entry *)buffer;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return "socket() failed";

	memset(request, 0, sizeof(struct route_entry));
	request->destination = (struct sockaddr *)to;

	if (ioctl(sock, SIOCGETRT, buffer, sizeof(buffer)) < 0) {
		close(sock);
		return "ioctl(SIOCGETRT) failed";
	}

	if (request->source != NULL && request->source->sa_family == AF_INET)
		memcpy(from, request->source, sizeof(struct sockaddr_in));

	close(sock);

	if (request->source == NULL || request->source->sa_family != AF_INET)
		return "No address available";

	return NULL;
}
