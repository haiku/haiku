/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_TRACING_H
#define KERNEL_SCHEDULER_TRACING_H


#include <arch/debug.h>
#include <cpu.h>
#include <thread.h>
#include <tracing.h>


#if SCHEDULER_TRACING

namespace SchedulerTracing {

class SchedulerTraceEntry : public AbstractTraceEntry {
public:
	SchedulerTraceEntry(Thread* thread)
		:
		fID(thread->id)
	{
	}

	thread_id ThreadID() const	{ return fID; }

	virtual const char* Name() const = 0;

protected:
	thread_id			fID;
};


class EnqueueThread : public SchedulerTraceEntry {
public:
	EnqueueThread(Thread* thread, Thread* previous, Thread* next)
		:
		SchedulerTraceEntry(thread),
		fPreviousID(-1),
		fNextID(-1),
		fPriority(thread->priority)
	{
		if (previous != NULL)
			fPreviousID = previous->id;
		if (next != NULL)
			fNextID = next->id;
		fName = alloc_tracing_buffer_strcpy(thread->name, B_OS_NAME_LENGTH,
			false);
		Initialized();
	}

	virtual void AddDump(TraceOutput& out);

	virtual const char* Name() const;

private:
	thread_id			fPreviousID;
	thread_id			fNextID;
	char*				fName;
	uint8				fPriority;
};


class RemoveThread : public SchedulerTraceEntry {
public:
	RemoveThread(Thread* thread)
		:
		SchedulerTraceEntry(thread),
		fPriority(thread->priority)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out);

	virtual const char* Name() const;

private:
	uint8				fPriority;
};


class ScheduleThread : public SchedulerTraceEntry {
public:
	ScheduleThread(Thread* thread, Thread* previous)
		:
		SchedulerTraceEntry(thread),
		fPreviousID(previous->id),
		fCPU(previous->cpu->cpu_num),
		fPriority(thread->priority),
		fPreviousState(previous->state),
		fPreviousWaitObjectType(previous->wait.type)
	{
		fName = alloc_tracing_buffer_strcpy(thread->name, B_OS_NAME_LENGTH,
			false);

#if SCHEDULER_TRACING >= 2
		if (fPreviousState == B_THREAD_READY)
			fPreviousPC = arch_debug_get_interrupt_pc(NULL);
		else
#endif
			fPreviousWaitObject = previous->wait.object;

		Initialized();
	}

	virtual void AddDump(TraceOutput& out);

	virtual const char* Name() const;

	thread_id PreviousThreadID() const		{ return fPreviousID; }
	uint8 PreviousState() const				{ return fPreviousState; }
	uint16 PreviousWaitObjectType() const	{ return fPreviousWaitObjectType; }
	const void* PreviousWaitObject() const	{ return fPreviousWaitObject; }

private:
	thread_id			fPreviousID;
	int32				fCPU;
	char*				fName;
	uint8				fPriority;
	uint8				fPreviousState;
	uint16				fPreviousWaitObjectType;
	union {
		const void*		fPreviousWaitObject;
		void*			fPreviousPC;
	};
};

}	// namespace SchedulerTracing

#	define T(x) new(std::nothrow) SchedulerTracing::x;
#else
#	define T(x) ;
#endif


#if SCHEDULER_TRACING

namespace SchedulerTracing {

enum ScheduleState {
	RUNNING,
	STILL_RUNNING,
	PREEMPTED,
	READY,
	WAITING,
	UNKNOWN
};

}

int cmd_scheduler(int argc, char** argv);

#endif	// SCHEDULER_TRACING

#endif	// KERNEL_SCHEDULER_TRACING_H
