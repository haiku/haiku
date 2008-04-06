/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfo.h"


SystemInfo::SystemInfo()
	:
	fTime(system_time())
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
