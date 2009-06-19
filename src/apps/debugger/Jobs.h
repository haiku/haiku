/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef JOBS_H
#define JOBS_H

#include "Worker.h"


class Architecture;
class CpuState;
class DebuggerInterface;
class Thread;


// job types
enum {
	JOB_TYPE_GET_CPU_STATE,
	JOB_TYPE_GET_STACK_TRACE
};


class GetCpuStateJob : public Job {
public:
								GetCpuStateJob(
									DebuggerInterface* debuggerInterface,
									Thread* thread);
	virtual						~GetCpuStateJob();

	virtual	JobKey				Key() const;
	virtual	status_t			Do();

private:
			DebuggerInterface*	fDebuggerInterface;
			Thread*				fThread;
};


class GetStackTraceJob : public Job {
public:
								GetStackTraceJob(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture,
									Thread* thread);
	virtual						~GetStackTraceJob();

	virtual	JobKey				Key() const;
	virtual	status_t			Do();

private:
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			Thread*				fThread;
			CpuState*			fCpuState;
};


#endif	// JOBS_H
