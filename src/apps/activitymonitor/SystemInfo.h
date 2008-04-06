/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H


#include <OS.h>


class SystemInfo {
public:
						SystemInfo();
						~SystemInfo();

			uint64		CachedMemory() const;
			uint64		UsedMemory() const;
			uint64		MaxMemory() const;

			uint32		UsedThreads() const;
			uint32		MaxThreads() const;

			uint32		UsedTeams() const;
			uint32		MaxTeams() const;

			bigtime_t	Time() const { return fTime; }
			const system_info& Info() const { return fSystemInfo; }

private:
	system_info			fSystemInfo;
	bigtime_t			fTime;
};

#endif	// SYSTEM_INFO_H
