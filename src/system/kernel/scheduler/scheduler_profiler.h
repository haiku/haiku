/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_PROFILER_H
#define KERNEL_SCHEDULER_PROFILER_H


#include <smp.h>


//#define SCHEDULER_PROFILING
#ifdef SCHEDULER_PROFILING


#define SCHEDULER_ENTER_FUNCTION()	\
	Scheduler::Profiling::Function schedulerProfiler(__PRETTY_FUNCTION__)

#define SCHEDULER_EXIT_FUNCTION()	\
	schedulerProfiler.Exit()


namespace Scheduler {

namespace Profiling {

class Profiler {
public:
							Profiler();

			void			EnterFunction(int32 cpu, const char* function);
			void			ExitFunction(int32 cpu, const char* function);

			void			DumpCalled(uint32 count);
			void			DumpTimeInclusive(uint32 count);
			void			DumpTimeExclusive(uint32 count);
			void			DumpTimeInclusivePerCall(uint32 count);
			void			DumpTimeExclusivePerCall(uint32 count);

			status_t		GetStatus() const	{ return fStatus; }

	static	Profiler*		Get();
	static	void			Initialize();

private:
	struct FunctionData {
			const char*		fFunction;

			uint32			fCalled;

			bigtime_t		fTimeInclusive;
			bigtime_t		fTimeExclusive;
	};

	struct FunctionEntry {
			FunctionData*	fFunction;

			nanotime_t		fEntryTime;
			nanotime_t		fOthersTime;
			nanotime_t		fProfilerTime;
	};

			uint32			_FunctionCount() const;
			void			_Dump(uint32 count);

			FunctionData*	_FindFunction(const char* function);

			template<typename Type, Type FunctionData::*Member>
	static	int				_CompareFunctions(const void* a, const void* b);

			template<typename Type, Type FunctionData::*Member>
	static	int				_CompareFunctionsPerCall(const void* a,
								const void* b);

			const uint32	kMaxFunctionEntries;
			const uint32	kMaxFunctionStackEntries;

			FunctionEntry*	fFunctionStacks[SMP_MAX_CPUS];
			uint32			fFunctionStackPointers[SMP_MAX_CPUS];

			FunctionData*	fFunctionData;
			spinlock		fFunctionLock;

			status_t		fStatus;
};

class Function {
public:
	inline					Function(const char* functionName);
	inline					~Function();

	inline	void			Exit();

private:
			const char*		fFunctionName;
};


Function::Function(const char* functionName)
	:
	fFunctionName(functionName)
{
	Profiler::Get()->EnterFunction(smp_get_current_cpu(), fFunctionName);
}


Function::~Function()
{
	if (fFunctionName != NULL)
		Exit();
}


void
Function::Exit()
{
	Profiler::Get()->ExitFunction(smp_get_current_cpu(), fFunctionName);
	fFunctionName = NULL;
}


}	// namespace Profiling

}	// namespace Scheduler


#else	// SCHEDULER_PROFILING

#define SCHEDULER_ENTER_FUNCTION()	(void)0
#define SCHEDULER_EXIT_FUNCTION()	(void)0

#endif	// !SCHEDULER_PROFILING


#endif	// KERNEL_SCHEDULER_PROFILER_H

