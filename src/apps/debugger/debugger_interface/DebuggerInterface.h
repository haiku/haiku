/*
 * Copyright 2009-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_INTERFACE_H
#define DEBUGGER_INTERFACE_H

#include <debugger.h>

#include <debug_support.h>
#include <ObjectList.h>

#include "TeamMemory.h"


class Architecture;
class AreaInfo;
class CpuState;
class DebugEvent;
class ElfSymbolLookup;
class ImageInfo;
class SemaphoreInfo;
class SymbolInfo;
class SystemInfo;
class TeamInfo;
class ThreadInfo;

namespace BPrivate {
class KMessage;
}


class DebuggerInterface : public TeamMemory {
public:
	virtual						~DebuggerInterface();

	virtual	status_t			Init()
									= 0;
	virtual	void				Close(bool killTeam) = 0;

	virtual	bool				Connected() const = 0;

	virtual	bool				IsPostMortem() const;

	virtual	team_id				TeamID() const = 0;

	virtual	Architecture*		GetArchitecture() const = 0;

	virtual	status_t			GetNextDebugEvent(DebugEvent*& _event) = 0;

	virtual	status_t			SetTeamDebuggingFlags(uint32 flags) = 0;

	virtual	status_t			ContinueThread(thread_id thread) = 0;
	virtual	status_t			StopThread(thread_id thread) = 0;
	virtual	status_t			SingleStepThread(thread_id thread) = 0;

	virtual	status_t			InstallBreakpoint(target_addr_t address) = 0;
	virtual	status_t			UninstallBreakpoint(target_addr_t address) = 0;

	virtual status_t			InstallWatchpoint(target_addr_t address,
									uint32 type, int32 length) = 0;
	virtual status_t			UninstallWatchpoint(target_addr_t address) = 0;

	virtual	status_t			GetSystemInfo(SystemInfo& info) = 0;
	virtual	status_t			GetTeamInfo(TeamInfo& info) = 0;
	virtual	status_t			GetThreadInfos(BObjectList<ThreadInfo>& infos)
									= 0;
	virtual	status_t			GetImageInfos(BObjectList<ImageInfo>& infos)
									= 0;
	virtual status_t			GetAreaInfos(BObjectList<AreaInfo>& infos)
									= 0;
	virtual status_t			GetSemaphoreInfos(
									BObjectList<SemaphoreInfo>& infos)
									= 0;

	virtual	status_t			GetSymbolInfos(team_id team, image_id image,
									BObjectList<SymbolInfo>& infos) = 0;
	virtual	status_t			GetSymbolInfo(team_id team, image_id image,
									const char* name, int32 symbolType,
									SymbolInfo& info) = 0;

	virtual	status_t			GetThreadInfo(thread_id thread,
									ThreadInfo& info) = 0;
	virtual	status_t			GetCpuState(thread_id thread,
									CpuState*& _state) = 0;
										// returns a reference to the caller
	virtual	status_t			SetCpuState(thread_id thread,
									const CpuState* state) = 0;

	virtual	status_t			GetCpuFeatures(uint32& flags) = 0;

	virtual	status_t			WriteCoreFile(const char* path) = 0;

	// TeamMemory
	virtual	status_t			GetMemoryProperties(target_addr_t address,
									uint32& protection, uint32& locking) = 0;

	virtual	ssize_t				ReadMemory(target_addr_t address, void* buffer,
									size_t size) = 0;
	virtual	ssize_t				WriteMemory(target_addr_t address,
									void* buffer, size_t size) = 0;

protected:
			status_t			GetElfSymbols(const char* filePath,
									int64 textDelta,
									BObjectList<SymbolInfo>& infos);
			status_t			GetElfSymbols(const void* symbolTable,
									uint32 symbolCount,
									uint32 symbolTableEntrySize,
									const char* stringTable,
									uint32 stringTableSize, bool is64Bit,
									bool swappedByteOrder, int64 textDelta,
									BObjectList<SymbolInfo>& infos);
			status_t			GetElfSymbols(ElfSymbolLookup* symbolLookup,
									BObjectList<SymbolInfo>& infos);

private:
			struct SymbolTableLookupSource;
};


#endif	// DEBUGGER_INTERFACE_H
