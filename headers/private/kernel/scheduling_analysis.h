/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SCHEDULING_ANALYSIS_H
#define _KERNEL_SCHEDULING_ANALYSIS_H

#include <tracing.h>
#include <thread_defs.h>


struct ConditionVariable;
struct mutex;
struct rw_lock;


#if SCHEDULING_ANALYSIS_TRACING
namespace SchedulingAnalysisTracing {

class WaitObjectTraceEntry : public AbstractTraceEntry {
public:
	virtual uint32 Type() const = 0;
	virtual void* Object() const = 0;
	virtual const char* Name() const = 0;

	virtual void* ReferencedObject() const
	{
		return NULL;
	}
};


class CreateSemaphore : public WaitObjectTraceEntry {
public:
	CreateSemaphore(sem_id id, const char* name)
		:
		fID(id),
		fName(alloc_tracing_buffer_strcpy(name, 128, false))
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("sem create \"%s\" -> %ld", fName, fID);
	}

	virtual uint32 Type() const
	{
		return THREAD_BLOCK_TYPE_SEMAPHORE;
	}

	virtual void* Object() const
	{
		return (void*)(addr_t)fID;
	}

	virtual const char* Name() const
	{
		return fName;
	}

private:
	sem_id		fID;
	const char*	fName;
};


class InitConditionVariable : public WaitObjectTraceEntry {
public:
	InitConditionVariable(ConditionVariable* variable, const void* object,
		const char* objectType)
		:
		fVariable(variable),
		fObject(object),
		fObjectType(alloc_tracing_buffer_strcpy(objectType, 128, false))
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("cvar init variable %p: object: %p \"%s\"", fVariable,
			fObject, fObjectType);
	}

	virtual uint32 Type() const
	{
		return THREAD_BLOCK_TYPE_CONDITION_VARIABLE;
	}

	virtual void* Object() const
	{
		return fVariable;
	}

	virtual const char* Name() const
	{
		return fObjectType;
	}

	virtual void* ReferencedObject() const
	{
		return (void*)fObject;
	}

private:
	ConditionVariable*	fVariable;
	const void*			fObject;
	const char*			fObjectType;
};


class InitMutex : public WaitObjectTraceEntry {
public:
	InitMutex(mutex* lock, const char* name)
		:
		fMutex(lock),
		fName(alloc_tracing_buffer_strcpy(name, 128, false))
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("mutex init %p: name: \"%s\"", fMutex, fName);
	}

	virtual uint32 Type() const
	{
		return THREAD_BLOCK_TYPE_MUTEX;
	}

	virtual void* Object() const
	{
		return fMutex;
	}

	virtual const char* Name() const
	{
		return fName;
	}

private:
	mutex*		fMutex;
	const char*	fName;
};


class InitRWLock : public WaitObjectTraceEntry {
public:
	InitRWLock(rw_lock* lock, const char* name)
		:
		fLock(lock),
		fName(alloc_tracing_buffer_strcpy(name, 128, false))
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("rwlock init %p: name: \"%s\"", fLock, fName);
	}

	virtual uint32 Type() const
	{
		return THREAD_BLOCK_TYPE_RW_LOCK;
	}

	virtual void* Object() const
	{
		return fLock;
	}

	virtual const char* Name() const
	{
		return fName;
	}

private:
	rw_lock*	fLock;
	const char*	fName;
};

}	// namespace SchedulingAnalysisTracing

#	define T_SCHEDULING_ANALYSIS(x) \
		new(std::nothrow) SchedulingAnalysisTracing::x;
#else
#	define T_SCHEDULING_ANALYSIS(x) ;
#endif	// SCHEDULING_ANALYSIS_TRACING

#endif	// _KERNEL_SCHEDULING_ANALYSIS_H
