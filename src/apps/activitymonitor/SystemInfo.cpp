/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfo.h"
#include "SystemInfoHandler.h"

#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>


SystemInfo::SystemInfo(SystemInfoHandler* handler)
	:
	fTime(system_time()),
	fRetrievedNetwork(false),
	fRunningApps(0),
	fClipboardSize(0),
	fClipboardTextSize(0),
	fMediaNodes(0),
	fMediaConnections(0),
	fMediaBuffers(0)
{
	get_system_info(&fSystemInfo);
	get_system_info_etc(B_MEMORY_INFO, &fMemoryInfo,
		sizeof(system_memory_info));

	if (handler != NULL) {
		fRunningApps = handler->RunningApps();
		fClipboardSize = handler->ClipboardSize();
		fClipboardTextSize = handler->ClipboardTextSize();
		fMediaNodes = handler->MediaNodes();
		fMediaConnections = handler->MediaConnections();
		fMediaBuffers = handler->MediaBuffers();
	}
}


SystemInfo::~SystemInfo()
{
}


uint64
SystemInfo::CachedMemory() const
{
#ifdef __HAIKU__
	return (uint64)fSystemInfo.cached_pages * B_PAGE_SIZE;
#else
	return 0LL;
#endif
}


uint64
SystemInfo::UsedMemory() const
{
	return (uint64)fSystemInfo.used_pages * B_PAGE_SIZE;
}


uint64
SystemInfo::MaxMemory() const
{
	return (uint64)fSystemInfo.max_pages * B_PAGE_SIZE;
}


uint32
SystemInfo::PageFaults() const
{
	return fMemoryInfo.page_faults;
}


uint64
SystemInfo::UsedSwapSpace() const
{
	return fMemoryInfo.max_swap_space - fMemoryInfo.free_swap_space;
}


uint64
SystemInfo::MaxSwapSpace() const
{
	return fMemoryInfo.max_swap_space;
}


uint32
SystemInfo::UsedSemaphores() const
{
	return fSystemInfo.used_sems;
}


uint32
SystemInfo::MaxSemaphores() const
{
	return fSystemInfo.max_sems;
}


uint32
SystemInfo::UsedPorts() const
{
	return fSystemInfo.used_ports;
}


uint32
SystemInfo::MaxPorts() const
{
	return fSystemInfo.max_ports;
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

#ifdef __HAIKU__
		if (ioctl(socket, SIOCGIFSTATS, &request, sizeof(struct ifreq)) == 0) {
			fBytesReceived += request.ifr_stats.receive.bytes;
			fBytesSent += request.ifr_stats.send.bytes;
		}
#endif

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


uint32
SystemInfo::UsedRunningApps() const
{
	return fRunningApps;
}


uint32
SystemInfo::MaxRunningApps() const
{
	return fSystemInfo.max_teams;
}


uint32
SystemInfo::ClipboardSize() const
{
	return fClipboardSize;
}


uint32
SystemInfo::ClipboardTextSize() const
{
	return fClipboardTextSize;
}


uint32
SystemInfo::MediaNodes() const
{
	return fMediaNodes;
}


uint32
SystemInfo::MediaConnections() const
{
	return fMediaConnections;
}


uint32
SystemInfo::MediaBuffers() const
{
	return fMediaBuffers;
}


