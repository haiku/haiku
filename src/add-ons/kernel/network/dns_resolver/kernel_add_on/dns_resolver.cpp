/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <net/dns_resolver.h>

#include <FindDirectory.h>
#include <lock.h>
#include <port.h>
#include <team.h>

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


static status_t
dns_resolve(const char* host, uint32* addr)
{
	mutex_lock(&gPortLock);
	do {
		status_t result = write_port(gPortRequest, MsgResolveRequest, host,
			strlen(host) + 1);
		if (result != B_OK) {
			result = dns_resolver_repair();
			if (result != B_OK) {
				mutex_unlock(&gPortLock);
				return result;
			}

			continue;
		}

		int32 code;
		result = read_port(gPortReply, &code, addr, sizeof(*addr));
		if (result < B_OK) {
			result = dns_resolver_repair();
			if (result != B_OK) {
				mutex_unlock(&gPortLock);
				return result;
			}

			continue;
		}
		mutex_unlock(&gPortLock);

		if (code == MsgResolveReply)
			return B_OK;
		else
			return *addr;

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

	dns_resolve,
};

module_info* modules[] = {
	(module_info*)&sDNSResolverModule,
	NULL
};

