/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef JOBS_H
#define JOBS_H


#include "ImageDebugInfoProvider.h"
#include "Types.h"
#include "Worker.h"


class Architecture;
class BVariant;
class CpuState;
class DebuggerInterface;
class Function;
class FunctionInstance;
class Image;
class StackFrame;
class StackFrameValues;
class Team;
class TeamMemory;
class TeamMemoryBlock;
class Thread;
class Type;
class TypeComponentPath;
class ValueLocation;
class ValueNode;
class ValueNodeChild;
class ValueNodeContainer;
class Variable;


// job types
enum {
	JOB_TYPE_GET_THREAD_STATE,
	JOB_TYPE_GET_CPU_STATE,
	JOB_TYPE_GET_STACK_TRACE,
	JOB_TYPE_LOAD_IMAGE_DEBUG_INFO,
	JOB_TYPE_LOAD_SOURCE_CODE,
	JOB_TYPE_GET_STACK_FRAME_VALUE,
	JOB_TYPE_RESOLVE_VALUE_NODE_VALUE,
	JOB_TYPE_GET_MEMORY_BLOCK
};


class GetThreadStateJob : public Job {
public:
								GetThreadStateJob(
									DebuggerInterface* debuggerInterface,
									Thread* thread);
	virtual						~GetThreadStateJob();

	virtual	const JobKey&		Key() const;
	virtual	status_t			Do();

private:
			SimpleJobKey		fKey;
			DebuggerInterface*	fDebuggerInterface;
			Thread*				fThread;
};


class GetCpuStateJob : public Job {
public:
								GetCpuStateJob(
									DebuggerInterface* debuggerInterface,
									Thread* thread);
	virtual						~GetCpuStateJob();

	virtual	const JobKey&		Key() const;
	virtual	status_t			Do();

private:
			SimpleJobKey		fKey;
			DebuggerInterface*	fDebuggerInterface;
			Thread*				fThread;
};


class GetStackTraceJob : public Job, private ImageDebugInfoProvider {
public:
								GetStackTraceJob(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture, Thread* thread);
	virtual						~GetStackTraceJob();

	virtual	const JobKey&		Key() const;
	virtual	status_t			Do();

private:
	// ImageDebugInfoProvider
	virtual	status_t			GetImageDebugInfo(Image* image,
									ImageDebugInfo*& _info);

private:
			SimpleJobKey		fKey;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			Thread*				fThread;
			CpuState*			fCpuState;
};


class LoadImageDebugInfoJob : public Job {
public:
								LoadImageDebugInfoJob(Image* image);
	virtual						~LoadImageDebugInfoJob();

	virtual	const JobKey&		Key() const;
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
			SimpleJobKey		fKey;
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

	virtual	const JobKey&		Key() const;
	virtual	status_t			Do();

private:
			SimpleJobKey		fKey;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			Team*				fTeam;
			FunctionInstance*	fFunctionInstance;
			bool				fLoadForFunction;
};


class ResolveValueNodeValueJob : public Job {
public:
								ResolveValueNodeValueJob(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture,
									CpuState* cpuState,
									ValueNodeContainer*	container,
									ValueNode* valueNode);
	virtual						~ResolveValueNodeValueJob();

	virtual	const JobKey&		Key() const;
	virtual	status_t			Do();

private:
			status_t			_ResolveNodeValue();
			status_t			_ResolveNodeChildLocation(
									ValueNodeChild* nodeChild);
			status_t			_ResolveParentNodeValue(ValueNode* parentNode);


private:
			SimpleJobKey		fKey;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			CpuState*			fCpuState;
			ValueNodeContainer*	fContainer;
			ValueNode*			fValueNode;
};


class RetrieveMemoryBlockJob : public Job {
public:
								RetrieveMemoryBlockJob(
									TeamMemory* teamMemory,
									TeamMemoryBlock* memoryBlock);
	virtual						~RetrieveMemoryBlockJob();

	virtual const JobKey&		Key() const;
	virtual status_t			Do();

private:
			SimpleJobKey		fKey;
			TeamMemory*			fTeamMemory;
			TeamMemoryBlock*	fMemoryBlock;
};


#endif	// JOBS_H
