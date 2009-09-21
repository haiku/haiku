/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef JOBS_H
#define JOBS_H


#include "ImageDebugInfoProvider.h"
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
class Thread;
class Type;
class TypeComponentPath;
class ValueLocation;
class Variable;


// job types
enum {
	JOB_TYPE_GET_THREAD_STATE,
	JOB_TYPE_GET_CPU_STATE,
	JOB_TYPE_GET_STACK_TRACE,
	JOB_TYPE_LOAD_IMAGE_DEBUG_INFO,
	JOB_TYPE_LOAD_SOURCE_CODE,
	JOB_TYPE_GET_STACK_FRAME_VALUE
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



struct GetStackFrameValueJobKey : JobKey {
	StackFrame*			stackFrame;
	Variable*			variable;
	TypeComponentPath*	path;

public:
								GetStackFrameValueJobKey(
									StackFrame* stackFrame,
									Variable* variable,
									TypeComponentPath* path);

	virtual	uint32				HashValue() const;

	virtual	bool				operator==(const JobKey& other) const;
};


class GetStackFrameValueJob : public Job {
public:
								GetStackFrameValueJob(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture,
									Thread* thread, StackFrame* stackFrame,
									Variable* variable,
									TypeComponentPath* path);
	virtual						~GetStackFrameValueJob();

	virtual	const JobKey&		Key() const;
	virtual	status_t			Do();

private:
			struct ValueJobKey;

private:
			status_t			_GetValue();
			status_t			_SetValue(const BVariant& value, Type* type,
									ValueLocation* location);
			status_t			_ResolveTypeAndLocation(Type*& _type,
									ValueLocation*& _location,
									bool& _valueResolved);
									// returns references
			status_t			_GetTypeLocationAndValue(
									TypeComponentPath* parentPath,
									Type*& _parentType,
									ValueLocation*& _parentLocation,
									BVariant& _parentValue);
									// returns references

private:
			GetStackFrameValueJobKey fKey;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			Thread*				fThread;
			StackFrame*			fStackFrame;
			Variable*			fVariable;
			TypeComponentPath*	fPath;
};


#endif	// JOBS_H
