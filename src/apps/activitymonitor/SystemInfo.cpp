/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfo.h"

#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include "SystemInfoHandler.h"


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
	__get_system_info_etc(B_MEMORY_INFO, &fMemoryInfo,
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

	BNetworkRoster& roster = BNetworkRoster::Default();

	BNetworkInterface interface;
	uint32 cookie = 0;
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		ifreq_stats stats;
		if (interface.GetStats(stats) == B_OK) {
			fBytesReceived += stats.receive.bytes;
			fBytesSent += stats.send.bytes;
		}
	}
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


