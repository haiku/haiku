/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CORE_FILE_DEBUGGER_INTERFACE_H
#define CORE_FILE_DEBUGGER_INTERFACE_H


#include "DebuggerInterface.h"
#include "TeamMemory.h"


class CoreFile;
struct CoreFileThreadInfo;
class ElfSection;
class ElfSymbolLookup;


class CoreFileDebuggerInterface : public DebuggerInterface {
public:
								CoreFileDebuggerInterface(CoreFile* coreFile);
	virtual						~CoreFileDebuggerInterface();

	virtual	status_t			Init();
	virtual	void				Close(bool killTeam);

	virtual	bool				Connected() const;

	virtual	bool				IsPostMortem() const;

	virtual	team_id				TeamID() const;

	virtual	Architecture*		GetArchitecture() const;

	virtual	status_t			GetNextDebugEvent(DebugEvent*& _event);

	virtual	status_t			SetTeamDebuggingFlags(uint32 flags);

	virtual	status_t			ContinueThread(thread_id thread);
	virtual	status_t			StopThread(thread_id thread);
	virtual	status_t			SingleStepThread(thread_id thread);

	virtual	status_t			InstallBreakpoint(target_addr_t address);
	virtual	status_t			UninstallBreakpoint(target_addr_t address);

	virtual status_t			InstallWatchpoint(target_addr_t address,
									uint32 type, int32 length);
	virtual status_t			UninstallWatchpoint(target_addr_t address);

	virtual	status_t			GetSystemInfo(SystemInfo& info);
	virtual	status_t			GetTeamInfo(TeamInfo& info);
	virtual	status_t			GetThreadInfos(BObjectList<ThreadInfo>& infos);
	virtual	status_t			GetImageInfos(BObjectList<ImageInfo>& infos);
	virtual status_t			GetAreaInfos(BObjectList<AreaInfo>& infos);
	virtual status_t			GetSemaphoreInfos(
									BObjectList<SemaphoreInfo>& infos);
	virtual	status_t			GetSymbolInfos(team_id team, image_id image,
									BObjectList<SymbolInfo>& infos);
	virtual	status_t			GetSymbolInfo(team_id team, image_id image,
									const char* name, int32 symbolType,
									SymbolInfo& info);

	virtual	status_t			GetThreadInfo(thread_id thread,
									ThreadInfo& info);
	virtual	status_t			GetCpuState(thread_id thread,
									CpuState*& _state);
										// returns a reference to the caller
	virtual	status_t			SetCpuState(thread_id thread,
									const CpuState* state);

	virtual	status_t			GetCpuFeatures(uint32& flags);

	virtual	status_t			WriteCoreFile(const char* path);

	// TeamMemory
	virtual	status_t			GetMemoryProperties(target_addr_t address,
									uint32& protection, uint32& locking);

	virtual	ssize_t				ReadMemory(target_addr_t address, void* buffer,
									size_t size);
	virtual	ssize_t				WriteMemory(target_addr_t address,
									void* buffer, size_t size);

private:
			void				_GetThreadInfo(
									const CoreFileThreadInfo& coreInfo,
									ThreadInfo& info);
			status_t			_CreateSharedObjectFileSymbolLookup(
									const char* path,
									ElfSymbolLookup*& _lookup);

private:
			CoreFile*			fCoreFile;
			Architecture*		fArchitecture;
};


#endif	// CORE_FILE_DEBUGGER_INTERFACE_H
