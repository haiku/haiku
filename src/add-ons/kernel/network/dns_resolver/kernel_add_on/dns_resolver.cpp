/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <net/dns_resolver.h>

#include <AutoDeleter.h>
#include <FindDirectory.h>
#include <lock.h>
#include <port.h>
#include <team.h>
#include <util/AutoLock.h>

#include "Definitions.h"


static 	mutex		gPortLock;
static	port_id		gPortRequest		= -1;
static	port_id		gPortReply			= -1;

static	const int32	kQueueLength		= 1;


static status_t
dns_resolver_repair()
{
	status_t result = B_OK;

	gPortRequest = create_port(kQueueLength, kPortNameReq);
	if (gPortRequest < B_OK)
		return gPortRequest;

	gPortReply = create_port(kQueueLength, kPortNameRpl);
	if (gPortReply < B_OK) {
		delete_port(gPortRequest);
		return gPortReply;
	}

	char path[256];
	if (find_directory(B_SYSTEM_SERVERS_DIRECTORY, static_cast<dev_t>(-1),
		false, path, sizeof(path)) != B_OK) {
		delete_port(gPortReply);
		delete_port(gPortRequest);
		return B_NAME_NOT_FOUND;
	}
	strlcat(path, "/dns_resolver_server", sizeof(path));

	const char* args[] = { path, NULL };
	thread_id thread = load_image_etc(1, args, NULL, B_NORMAL_PRIORITY,
		B_SYSTEM_TEAM, 0);
	if (thread < B_OK) {
		delete_port(gPortReply);
		delete_port(gPortRequest);
		return thread;
	}

	set_port_owner(gPortRequest, thread);
	set_port_owner(gPortReply, thread);

	result = resume_thread(thread);
	if (result != B_OK) {
		kill_thread(thread);
		delete_port(gPortReply);
		delete_port(gPortRequest);
		return result;
	}

	return B_OK;
}


static status_t
dns_resolver_init()
{
	mutex_init(&gPortLock, NULL);
	return dns_resolver_repair();
}


static status_t
dns_resolver_uninit()
{
	delete_port(gPortRequest);
	delete_port(gPortReply);
	mutex_destroy(&gPortLock);

	return B_OK;
}


static void
RelocateEntries(struct addrinfo *addr)
{
	char* generalOffset = reinterpret_cast<char*>(addr);

	struct addrinfo* current = addr;
	while (current != NULL) {
		uint64 addrOffset = reinterpret_cast<uint64>(current->ai_addr);
		uint64 nameOffset = reinterpret_cast<uint64>(current->ai_canonname);
		uint64 nextOffset = reinterpret_cast<uint64>(current->ai_next);

		if (current->ai_addr != NULL) {
			current->ai_addr =
				reinterpret_cast<sockaddr*>(generalOffset + addrOffset);
		}

		if (current->ai_canonname != NULL)
			current->ai_canonname = generalOffset + nameOffset;

		if (current->ai_next != NULL) {
			current->ai_next =
				reinterpret_cast<addrinfo*>(generalOffset + nextOffset);
		}

		current = current->ai_next;
	}
}


static status_t
GetAddrInfo(const char* node, const char* service,
	const struct addrinfo* hints, struct addrinfo** res)
{
	uint32 nodeSize = node != NULL ? strlen(node) + 1 : 1;
	uint32 serviceSize = service != NULL ? strlen(service) + 1 : 1;
	uint32 size = nodeSize + serviceSize + sizeof(*hints);
	char* buffer = reinterpret_cast<char*>(malloc(size));
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter _(buffer);

	off_t off = 0;
	if (node != NULL)
		strcpy(buffer + off, node);
	else
		buffer[off] = '\0';
	off += nodeSize;

	if (service != NULL)
		strcpy(buffer + off, service);
	else
		buffer[off] = '\0';
	off += serviceSize;

	if (hints != NULL)
		memcpy(buffer + off, hints, sizeof(*hints));
	else {
		struct addrinfo *nullHints =
			reinterpret_cast<struct addrinfo*>(buffer + off);
		memset(nullHints, 0, sizeof(*nullHints));
		nullHints->ai_family = AF_UNSPEC;
	}

	MutexLocker locker(gPortLock);
	do {
		status_t result = write_port(gPortRequest, MsgGetAddrInfo, buffer,
			size);
		if (result != B_OK) {
			result = dns_resolver_repair();
			if (result != B_OK)
				return result;
			continue;
		}

		ssize_t replySize = port_buffer_size(gPortReply);
		if (replySize < B_OK) {
			result = dns_resolver_repair();
			if (result != B_OK)
				return result;
			continue;
		}

		void* reply = malloc(replySize);
		if (reply == NULL)
			return B_NO_MEMORY;

		int32 code;
		replySize = read_port(gPortReply, &code, reply, replySize);
		if (replySize < B_OK) {
			result = dns_resolver_repair();
			if (result != B_OK) {
				free(reply);
				return result;
			}
			continue;
		}

		struct addrinfo *addr;
		switch (code) {
			case MsgReply:
				addr = reinterpret_cast<struct addrinfo*>(reply);
				RelocateEntries(addr);
				*res = addr;
				return B_OK;
			case MsgError:
				result = *reinterpret_cast<status_t*>(reply);
				free(reply);
				return result;
			default:
				free(reply);
				return B_BAD_VALUE;
		}

	} while (true);
}


static status_t
dns_resolver_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return dns_resolver_init();
		case B_MODULE_UNINIT:
			return dns_resolver_uninit();
		default:
			return B_ERROR;
	}
}

static dns_resolver_module sDNSResolverModule = {
	{
		DNS_RESOLVER_MODULE_NAME,
		0,
		dns_resolver_std_ops,
	},

	GetAddrInfo,
};

module_info* modules[] = {
	(module_info*)&sDNSResolverModule,
	NULL
};

