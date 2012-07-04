/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>

#include <AutoDeleter.h>
#include <OS.h>
#include <SupportDefs.h>

#include "Definitions.h"


port_id		gRequestPort;
port_id		gReplyPort;


status_t
GetAddrInfo(const char* buffer)
{
	const char* node = buffer[0] == '\0' ? NULL : buffer;
	uint32 nodeSize = node != NULL ? strlen(node) + 1 : 1;

	const char* service = buffer[nodeSize] == '\0' ? NULL : buffer + nodeSize;
	uint32 serviceSize = service != NULL ? strlen(service) + 1 : 1;

	const struct addrinfo* hints =
		reinterpret_cast<const addrinfo*>(buffer + nodeSize + serviceSize);

	struct addrinfo* ai;
	status_t result = getaddrinfo(node, service, hints, &ai);
	if (result != B_OK)
		return write_port(gReplyPort, MsgError, &result, sizeof(result));

	uint32 addrsSize = ai == NULL ? 0 : sizeof(addrinfo);
	uint32 namesSize = 0;
	uint32 socksSize = 0;

	addrinfo* current = ai;
	while (current != NULL) {
		if (current->ai_canonname != NULL)
			namesSize += strlen(current->ai_canonname) + 1;
		if (current->ai_addr != NULL) {
			if (current->ai_family == AF_INET)
				socksSize += sizeof(sockaddr_in);
			else
				socksSize += sizeof(sockaddr_in6);
		}
		if (current->ai_next != NULL)
			addrsSize += sizeof(addrinfo);
		current = current->ai_next;
	}

	uint32 totalSize = addrsSize + namesSize + socksSize;
	char* reply = reinterpret_cast<char*>(malloc(totalSize));
	if (reply == NULL) {
		free(reply);

		result = B_NO_MEMORY;
		return write_port(gReplyPort, MsgError, &result, sizeof(result));
	}

	uint32 addrPos = 0;
	uint32 namePos = addrsSize;
	uint32 sockPos = addrsSize + namesSize;

	current = ai;
	while (current != NULL) {
		if (current->ai_canonname != NULL) {
			strcpy(reply + namePos, current->ai_canonname);
			uint32 nSize = strlen(current->ai_canonname) + 1;
			current->ai_canonname = reinterpret_cast<char*>(namePos);
			namePos += nSize;
		}
		if (current->ai_addr != NULL) {
			if (current->ai_family == AF_INET) {
				memcpy(reply + sockPos, current->ai_addr, sizeof(sockaddr_in));
				current->ai_addr = reinterpret_cast<sockaddr*>(sockPos);
				sockPos += sizeof(sockaddr_in);
			} else {
				memcpy(reply + sockPos, current->ai_addr, sizeof(sockaddr_in6));
				current->ai_addr = reinterpret_cast<sockaddr*>(sockPos);
				sockPos += sizeof(sockaddr_in6);
			}
		}

		addrinfo* next = current->ai_next;
		current->ai_next = reinterpret_cast<addrinfo*>(addrPos) + 1;
		memcpy(reply + addrPos, current, sizeof(addrinfo));
		addrPos += sizeof(addrinfo);

		current = next;
	}

	return write_port(gReplyPort, MsgReply, reply, totalSize);
}


status_t
MainLoop()
{
	do {
		ssize_t size = port_buffer_size(gRequestPort);
		if (size < B_OK)
			return 0;

		void* buffer = malloc(size);
		if (buffer == NULL)
			return B_NO_MEMORY;
		MemoryDeleter _(buffer);

		int32 code;
		size = read_port(gRequestPort, &code, buffer, size);
		if (size < B_OK)
			return 0;

		status_t result;
		switch (code) {
			case MsgGetAddrInfo:
				result = GetAddrInfo(reinterpret_cast<char*>(buffer));
			default:
				result = B_BAD_VALUE;
				write_port(gReplyPort, MsgError, &result, sizeof(result));
				result = B_OK;
		}

		if (result != B_OK)
			return 0;
	} while (true);
}


int
main(int argc, char** argv)
{
	gRequestPort = find_port(kPortNameReq);
	if (gRequestPort < B_OK) {
		fprintf(stderr, "%s\n", strerror(gRequestPort));
		return gRequestPort;
	}

	gReplyPort = find_port(kPortNameRpl);
	if (gReplyPort < B_OK) {
		fprintf(stderr, "%s\n", strerror(gReplyPort));
		return gReplyPort;
	}

	return MainLoop();
}

