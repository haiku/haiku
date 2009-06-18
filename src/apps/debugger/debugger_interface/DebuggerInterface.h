/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_INTERFACE_H
#define DEBUGGER_INTERFACE_H

#include <debugger.h>

#include <debug_support.h>
#include <ObjectList.h>


class Architecture;
class CpuState;
class DebugEvent;
class ImageInfo;
class ThreadInfo;


class DebuggerInterface {
public:
								DebuggerInterface(team_id teamID);
								~DebuggerInterface();

			status_t			Init();
			void				Close();

			Architecture*		GetArchitecture() const
									{ return fArchitecture; }

	virtual	status_t			GetNextDebugEvent(DebugEvent*& _event);

	virtual	status_t			SetTeamDebuggingFlags(uint32 flags);

	virtual	status_t			ContinueThread(thread_id thread);
	virtual	status_t			StopThread(thread_id thread);
	virtual	status_t			SingleStepThread(thread_id thread);

	virtual	status_t			GetThreadInfos(BObjectList<ThreadInfo>& infos);
	virtual	status_t			GetImageInfos(BObjectList<ImageInfo>& infos);

	virtual	status_t			GetThreadInfo(thread_id thread,
									ThreadInfo& info);
	virtual	status_t			GetCpuState(thread_id thread,
									CpuState*& _state);
										// returns a reference to the caller

private:
	struct DebugContext;
	struct DebugContextPool;
	struct DebugContextGetter;

private:
			status_t			_CreateDebugEvent(int32 messageCode,
									const debug_debugger_message_data& message,
									bool& _ignore, DebugEvent*& _event);

private:
			team_id				fTeamID;
			port_id				fDebuggerPort;
			port_id				fNubPort;
			DebugContextPool*	fDebugContextPool;
			Architecture*		fArchitecture;
};

#endif	// DEBUGGER_INTERFACE_H
