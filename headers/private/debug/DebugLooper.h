/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEBUG_LOOPER_H
#define _DEBUG_LOOPER_H


#include <Locker.h>

#include <ObjectList.h>


class BDebugMessageHandler;
class BTeamDebugger;


class BDebugLooper {
public:
								BDebugLooper();
	virtual						~BDebugLooper();

			status_t			Init();

			thread_id			Run(bool spawnThread);
			void				Quit();

			status_t			AddTeamDebugger(BTeamDebugger* debugger,
									BDebugMessageHandler* handler);
			bool				RemoveTeamDebugger(BTeamDebugger* debugger);
			bool				RemoveTeamDebugger(team_id team);

private:
			struct Debugger;

			struct Job;
			struct JobList;
			struct AddDebuggerJob;
			struct RemoveDebuggerJob;

			friend struct AddDebuggerJob;
			friend struct RemoveDebuggerJob;

			typedef BObjectList<Debugger> DebuggerList;

private:
	static	status_t			_MessageLoopEntry(void* data);
			status_t			_MessageLoop();

			status_t			_DoJob(Job* job);
			void				_Notify();

private:
			BLocker				fLock;
			thread_id			fThread;
			bool				fOwnsThread;
			bool				fTerminating;
			bool				fNotified;
			JobList*			fJobs;
			sem_id				fEventSemaphore;
			DebuggerList		fDebuggers;
};


#endif	// _DEBUG_LOOPER_H
