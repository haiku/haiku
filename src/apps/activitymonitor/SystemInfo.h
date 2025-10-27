/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H


#include <OS.h>
#include <String.h>

#include <system_info.h>


class SystemInfoHandler;


class SystemInfo {
public:
						SystemInfo(SystemInfoHandler* handler = NULL);
						~SystemInfo();

			uint64		CachedMemory() const;
			uint64		BlockCacheMemory() const;
			uint64		UsedMemory() const;
			uint64		MaxMemory() const;

			uint32		PageFaults() const;

			uint64		MaxSwapSpace() const;
			uint64		UsedSwapSpace() const;

			uint32		UsedSemaphores() const;
			uint32		MaxSemaphores() const;

			uint32		UsedPorts() const;
			uint32		MaxPorts() const;

			uint32		UsedThreads() const;
			uint32		MaxThreads() const;

			uint32		UsedTeams() const;
			uint32		MaxTeams() const;

			bigtime_t	Time() const { return fTime; }
			uint32		CPUCount() const { return fSystemInfo.cpu_count; }
			bigtime_t	CPUActiveTime(uint32 cpu) const
							{ return fCPUInfos[cpu].active_time; }
			uint64		CPUCurrentFrequency(uint32 cpu) const
							{ return fCPUInfos[cpu].current_frequency; }
			const system_info& Info() const { return fSystemInfo; }

			uint64		NetworkReceived();
			uint64		NetworkSent();

			uint32		UsedRunningApps() const;
			uint32		MaxRunningApps() const;

			uint32		ClipboardSize() const;
			uint32		ClipboardTextSize() const;

			uint32		MediaNodes() const;
			uint32		MediaConnections() const;
			uint32		MediaBuffers() const;

			float		Temperature() const { return fTemperature; }
	const	BString&	ThermalZone() const { return fThermalZone; }
private:
			void		_RetrieveNetwork();

	system_info			fSystemInfo;
	cpu_info*			fCPUInfos;
	bigtime_t			fTime;
	bool				fRetrievedNetwork;
	uint64				fBytesReceived;
	uint64				fBytesSent;
	uint32				fRunningApps;
	uint32				fClipboardSize;
	uint32				fClipboardTextSize;
	uint32				fMediaNodes;
	uint32				fMediaConnections;
	uint32				fMediaBuffers;
	float				fTemperature;
	BString				fThermalZone;
};

#endif	// SYSTEM_INFO_H
