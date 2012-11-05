/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_INTERFACE_H
#define DEBUGGER_INTERFACE_H

#include <debugger.h>

#include <debug_support.h>
#include <ObjectList.h>

#include "TeamMemory.h"


class Architecture;
class CpuState;
class DebugEvent;
class ImageInfo;
class SymbolInfo;
class ThreadInfo;

namespace BPrivate {
class KMessage;
}


class DebuggerInterface : public TeamMemory {
public:
								DebuggerInterface(team_id teamID);
	virtual						~DebuggerInterface();

			status_t			Init();
			void				Close(bool killTeam);

			Architecture*		GetArchitecture() const
									{ return fArchitecture; }

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

	virtual	status_t			GetThreadInfos(BObjectList<ThreadInfo>& infos);
	virtual	status_t			GetImageInfos(BObjectList<ImageInfo>& infos);
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

	// TeamMemory
	virtual	ssize_t				ReadMemory(target_addr_t address, void* buffer,
									size_t size);
	virtual	ssize_t				WriteMemory(target_addr_t address,
									void* buffer, size_t size);

private:
	struct DebugContext;
	struct DebugContextPool;
	struct DebugContextGetter;

private:
			status_t			_CreateDebugEvent(int32 messageCode,
									const debug_debugger_message_data& message,
									bool& _ignore, DebugEvent*& _event);

			status_t			_GetNextSystemWatchEvent(DebugEvent*& _event,
									BPrivate::KMessage& message);

private:
			team_id				fTeamID;
			port_id				fDebuggerPort;
			port_id				fNubPort;
			DebugContextPool*	fDebugContextPool;
			Architecture*		fArchitecture;
};

#endif	// DEBUGGER_INTERFACE_H
