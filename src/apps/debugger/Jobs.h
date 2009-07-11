/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef JOBS_H
#define JOBS_H

#include "ImageDebugInfoProvider.h"
#include "Worker.h"


class Architecture;
class CpuState;
class DebuggerInterface;
class Function;
class FunctionInstance;
class Image;
class StackFrame;
class Team;
class Thread;


// job types
enum {
	JOB_TYPE_GET_THREAD_STATE,
	JOB_TYPE_GET_CPU_STATE,
	JOB_TYPE_GET_STACK_TRACE,
	JOB_TYPE_LOAD_IMAGE_DEBUG_INFO,
	JOB_TYPE_LOAD_SOURCE_CODE
};


class GetThreadStateJob : public Job {
public:
								GetThreadStateJob(
									DebuggerInterface* debuggerInterface,
									Thread* thread);
	virtual						~GetThreadStateJob();

	virtual	JobKey				Key() const;
	virtual	status_t			Do();

private:
			DebuggerInterface*	fDebuggerInterface;
			Thread*				fThread;
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


class GetStackTraceJob : public Job, private ImageDebugInfoProvider {
public:
								GetStackTraceJob(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture, Thread* thread);
	virtual						~GetStackTraceJob();

	virtual	JobKey				Key() const;
	virtual	status_t			Do();

private:
	// ImageDebugInfoProvider
	virtual	status_t			GetImageDebugInfo(Image* image,
									ImageDebugInfo*& _info);

private:
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			Thread*				fThread;
			CpuState*			fCpuState;
};


class LoadImageDebugInfoJob : public Job {
public:
								LoadImageDebugInfoJob(Image* image);
	virtual						~LoadImageDebugInfoJob();

	virtual	JobKey				Key() const;
	virtual	status_t			Do();

	static	status_t			ScheduleIfNecessary(Worker* worker,
									Image* image,
									ImageDebugInfo** _imageDebugInfo = NULL);
										// If already loaded returns a
										// reference, if desired. If not loaded
										// schedules a job, but does not wait;
										// returns B_OK and NULL. An error,
										// if scheduling the job failed, or the
										// debug info already failed to load
										// earlier.

private:
			Image*				fImage;
};


class LoadSourceCodeJob : public Job {
public:
								LoadSourceCodeJob(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture, Team* team,
									FunctionInstance* functionInstance,
									bool loadForFunction);
	virtual						~LoadSourceCodeJob();

	virtual	JobKey				Key() const;
	virtual	status_t			Do();

private:
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			Team*				fTeam;
			FunctionInstance*	fFunctionInstance;
			bool				fLoadForFunction;
};


#endif	// JOBS_H
