/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <stdlib.h>

#include <sys/socket.h>
#include <netdb.h>

#include <port.h>
#include <SupportDefs.h>

#include "Definitions.h"


status_t
resolve_dns(const char* host, uint32* addr)
{
	addrinfo* ai;
	addrinfo* current;
	status_t result = getaddrinfo(host, NULL, NULL, &ai);
	if (result != B_OK)
		return result;

	current = ai;

	while (current != NULL) {
		if (current->ai_family == AF_INET) {
			sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(current->ai_addr);
			*addr = sin->sin_addr.s_addr;
			freeaddrinfo(ai);
			return B_OK;
		}

		current = current->ai_next;
	}

	freeaddrinfo(ai);
	return B_NAME_NOT_FOUND;
}


status_t
main_loop(port_id portReq, port_id portRpl)
{
	do {
		ssize_t size = port_buffer_size(portReq);
		if (size < B_OK)
			return 0;

		void* buffer = malloc(size);
		if (buffer == NULL)
			return B_NO_MEMORY;

		int32 code;
		status_t result = read_port(portReq, &code, buffer, size);
		if (size < B_OK) {
			free(buffer);
			return 0;
		}

		if (code != MsgResolveRequest) {
			free(buffer);
			continue;
		}

		uint32 addr;
		result = resolve_dns(reinterpret_cast<char*>(buffer), &addr);
		free(buffer);

		if (result == B_OK)
			result = write_port(portRpl, MsgResolveReply, &addr, sizeof(addr));
		else {
			result = write_port(portRpl, MsgResolveError, &result,
				sizeof(result));
		}

		if (result == B_BAD_PORT_ID)
			return 0;

	} while (true);
}


int
main(int argc, char** argv)
{
	port_id portReq = find_port(kPortNameReq);
	if (portReq == B_NAME_NOT_FOUND) {
		fprintf(stderr, "%s\n", strerror(portReq));
		return portReq;
	}

	port_id portRpl = find_port(kPortNameRpl);
	if (portRpl == B_NAME_NOT_FOUND) {
		fprintf(stderr, "%s\n", strerror(portRpl));
		return portRpl;
	}

	return main_loop(portReq, portRpl);
}

