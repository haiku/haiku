/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_HANDLER_H
#define THREAD_HANDLER_H

#include <Referenceable.h>
#include <util/OpenHashTable.h>

#include "DebugEvent.h"
#include "Thread.h"


class DebuggerInterface;
class TeamDebugModel;
class Worker;


class ThreadHandler : public Referenceable,
	public HashTableLink<ThreadHandler> {
public:
								ThreadHandler(TeamDebugModel* debugModel,
									Thread* thread, Worker* worker,
									DebuggerInterface* debuggerInterface);
								~ThreadHandler();

			void				Init();

			thread_id			ThreadID() const	{ return fThread->ID(); }
			Thread*				GetThread() const	{ return fThread; }

			bool				HandleThreadDebugged(
									ThreadDebuggedEvent* event);
			bool				HandleDebuggerCall(
									DebuggerCallEvent* event);
			bool				HandleBreakpointHit(
									BreakpointHitEvent* event);
			bool				HandleWatchpointHit(
									WatchpointHitEvent* event);
			bool				HandleSingleStep(
									SingleStepEvent* event);
			bool				HandleExceptionOccurred(
									ExceptionOccurredEvent* event);

			void				HandleThreadAction(uint32 action);

			void				HandleThreadStateChanged();
			void				HandleCpuStateChanged();
			void				HandleStackTraceChanged();

private:
			bool				_HandleThreadStopped(CpuState* cpuState);

			void				_SetThreadState(uint32 state,
									CpuState* cpuState);

private:
			TeamDebugModel*		fDebugModel;
			Thread*				fThread;
			Worker*				fWorker;
			DebuggerInterface*	fDebuggerInterface;
};


struct ThreadHandlerHashDefinition {
	typedef thread_id		KeyType;
	typedef	ThreadHandler	ValueType;

	size_t HashKey(thread_id key) const
	{
		return (size_t)key;
	}

	size_t Hash(const ThreadHandler* value) const
	{
		return HashKey(value->ThreadID());
	}

	bool Compare(thread_id key, ThreadHandler* value) const
	{
		return value->ThreadID() == key;
	}

	HashTableLink<ThreadHandler>* GetLink(ThreadHandler* value) const
	{
		return value;
	}
};

typedef OpenHashTable<ThreadHandlerHashDefinition> ThreadHandlerTable;


#endif	// THREAD_HANDLER_H
