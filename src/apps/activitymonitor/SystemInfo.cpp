/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfo.h"

#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>


SystemInfo::SystemInfo()
	:
	fTime(system_time()),
	fRetrievedNetwork(false)
{
	get_system_info(&fSystemInfo);
}


SystemInfo::~SystemInfo()
{
}


uint64
SystemInfo::CachedMemory() const
{
	return fSystemInfo.cached_pages * B_PAGE_SIZE;
}


uint64
SystemInfo::UsedMemory() const
{
	return fSystemInfo.used_pages * B_PAGE_SIZE;
}


uint64
SystemInfo::MaxMemory() const
{
	return fSystemInfo.max_pages * B_PAGE_SIZE;
}


uint32
SystemInfo::UsedThreads() const
{
	return fSystemInfo.used_threads;
}


uint32
SystemInfo::MaxThreads() const
{
	return fSystemInfo.max_threads;
}


uint32
SystemInfo::UsedTeams() const
{
	return fSystemInfo.used_teams;
}


uint32
SystemInfo::MaxTeams() const
{
	return fSystemInfo.max_teams;
}


void
SystemInfo::_RetrieveNetwork()
{
	if (fRetrievedNetwork)
		return;

	fBytesReceived = 0;
	fBytesSent = 0;
	fRetrievedNetwork = true;

	// we need a socket to talk to the networking stack
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return;

	// get a list of all interfaces

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0
		|| config.ifc_value == 0) {
		close(socket);
		return;
	}

	uint32 count = (uint32)config.ifc_value;

	void *buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL) {
		close(socket);
		return;
	}

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0) {
		close(socket);
		return;
	}

	ifreq *interface = (ifreq *)buffer;

	for (uint32 i = 0; i < count; i++) {
		ifreq request;
		strlcpy(request.ifr_name, interface->ifr_name, IF_NAMESIZE);

		if (ioctl(socket, SIOCGIFSTATS, &request, sizeof(struct ifreq)) == 0) {
			fBytesReceived += request.ifr_stats.receive.bytes;
			fBytesSent += request.ifr_stats.send.bytes;
		}

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE
			+ interface->ifr_addr.sa_len);
	}

	free(buffer);
	close(socket);
}


uint64
SystemInfo::NetworkReceived()
{
	_RetrieveNetwork();
	return fBytesReceived;
}


uint64
SystemInfo::NetworkSent()
{
	_RetrieveNetwork();
	return fBytesSent;
}
