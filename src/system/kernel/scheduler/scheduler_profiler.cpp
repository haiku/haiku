/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */

#include "scheduler_profiler.h"

#include <debug.h>
#include <util/AutoLock.h>

#include <algorithm>


#ifdef SCHEDULER_PROFILING


using namespace Scheduler;
using namespace Scheduler::Profiling;


static Profiler* sProfiler;

static int dump_profiler(int argc, char** argv);


Profiler::Profiler()
	:
	kMaxFunctionEntries(1024),
	kMaxFunctionStackEntries(512),
	fFunctionData(new(std::nothrow) FunctionData[kMaxFunctionEntries]),
	fStatus(B_OK)
{
	B_INITIALIZE_SPINLOCK(&fFunctionLock);

	if (fFunctionData == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}
	memset(fFunctionData, 0, sizeof(FunctionData) * kMaxFunctionEntries);

	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		fFunctionStacks[i]
			= new(std::nothrow) FunctionEntry[kMaxFunctionStackEntries];
		if (fFunctionStacks[i] == NULL) {
			fStatus = B_NO_MEMORY;
			return;
		}
		memset(fFunctionStacks[i], 0,
			sizeof(FunctionEntry) * kMaxFunctionStackEntries);
	}
	memset(fFunctionStackPointers, 0, sizeof(int32) * smp_get_num_cpus());
}


void
Profiler::EnterFunction(int32 cpu, const char* functionName)
{
	nanotime_t start = system_time_nsecs();

	FunctionData* function = _FindFunction(functionName);
	if (function == NULL)
		return;
	atomic_add((int32*)&function->fCalled, 1);

	FunctionEntry* stackEntry
		= &fFunctionStacks[cpu][fFunctionStackPointers[cpu]];
	fFunctionStackPointers[cpu]++;

	ASSERT(fFunctionStackPointers[cpu] < kMaxFunctionStackEntries);

	stackEntry->fFunction = function;
	stackEntry->fEntryTime = start;
	stackEntry->fOthersTime = 0;

	nanotime_t stop = system_time_nsecs();
	stackEntry->fProfilerTime = stop - start;
}


void
Profiler::ExitFunction(int32 cpu, const char* functionName)
{
	nanotime_t start = system_time_nsecs();

	ASSERT(fFunctionStackPointers[cpu] > 0);
	fFunctionStackPointers[cpu]--;
	FunctionEntry* stackEntry
		= &fFunctionStacks[cpu][fFunctionStackPointers[cpu]];

	nanotime_t timeSpent = start - stackEntry->fEntryTime;
	timeSpent -= stackEntry->fProfilerTime;

	atomic_add64(&stackEntry->fFunction->fTimeInclusive, timeSpent);
	atomic_add64(&stackEntry->fFunction->fTimeExclusive,
		timeSpent - stackEntry->fOthersTime);

	nanotime_t profilerTime = stackEntry->fProfilerTime;
	if (fFunctionStackPointers[cpu] > 0) {
		stackEntry = &fFunctionStacks[cpu][fFunctionStackPointers[cpu] - 1];
		stackEntry->fOthersTime += timeSpent;
		stackEntry->fProfilerTime += profilerTime;

		nanotime_t stop = system_time_nsecs();
		stackEntry->fProfilerTime += stop - start;
	}
}


void
Profiler::DumpCalled(uint32 maxCount)
{
	uint32 count = _FunctionCount();

	qsort(fFunctionData, count, sizeof(FunctionData),
		&_CompareFunctions<uint32, &FunctionData::fCalled>);

	if (maxCount > 0)
		count = std::min(count, maxCount);
	_Dump(count);
}


void
Profiler::DumpTimeInclusive(uint32 maxCount)
{
	uint32 count = _FunctionCount();

	qsort(fFunctionData, count, sizeof(FunctionData),
		&_CompareFunctions<nanotime_t, &FunctionData::fTimeInclusive>);

	if (maxCount > 0)
		count = std::min(count, maxCount);
	_Dump(count);
}


void
Profiler::DumpTimeExclusive(uint32 maxCount)
{
	uint32 count = _FunctionCount();

	qsort(fFunctionData, count, sizeof(FunctionData),
		&_CompareFunctions<nanotime_t, &FunctionData::fTimeExclusive>);

	if (maxCount > 0)
		count = std::min(count, maxCount);
	_Dump(count);
}


void
Profiler::DumpTimeInclusivePerCall(uint32 maxCount)
{
	uint32 count = _FunctionCount();

	qsort(fFunctionData, count, sizeof(FunctionData),
		&_CompareFunctionsPerCall<nanotime_t, &FunctionData::fTimeInclusive>);

	if (maxCount > 0)
		count = std::min(count, maxCount);
	_Dump(count);
}


void
Profiler::DumpTimeExclusivePerCall(uint32 maxCount)
{
	uint32 count = _FunctionCount();

	qsort(fFunctionData, count, sizeof(FunctionData),
		&_CompareFunctionsPerCall<nanotime_t, &FunctionData::fTimeExclusive>);

	if (maxCount > 0)
		count = std::min(count, maxCount);
	_Dump(count);
}


/* static */ Profiler*
Profiler::Get()
{
	return sProfiler;
}


/* static */ void
Profiler::Initialize()
{
	sProfiler = new(std::nothrow) Profiler;
	if (sProfiler == NULL || sProfiler->GetStatus() != B_OK)
		panic("Scheduler::Profiling::Profiler: could not initialize profiler");

	add_debugger_command_etc("scheduler_profiler", &dump_profiler,
		"Show data collected by scheduler profiler",
		"[ <field> [ <count> ] ]\n"
		"Shows data collected by scheduler profiler\n"
		"  <field>   - Field used to sort functions. Available: called,"
			" time-inclusive, time-inclusive-per-call, time-exclusive,"
			" time-exclusive-per-call.\n"
		"              (defaults to \"called\")\n"
		"  <count>   - Maximum number of showed functions.\n", 0);
}


uint32
Profiler::_FunctionCount() const
{
	uint32 count;
	for (count = 0; count < kMaxFunctionEntries; count++) {
		if (fFunctionData[count].fFunction == NULL)
			break;
	}
	return count;
}


void
Profiler::_Dump(uint32 count)
{
	kprintf("Function calls (%" B_PRId32 " functions):\n", count);
	kprintf("    called time-inclusive per-call time-exclusive per-call "
		"function\n");
	for (uint32 i = 0; i < count; i++) {
		FunctionData* function = &fFunctionData[i];
		kprintf("%10" B_PRId32 " %14" B_PRId64 " %8" B_PRId64 " %14" B_PRId64
			" %8" B_PRId64 " %s\n", function->fCalled,
			function->fTimeInclusive,
			function->fTimeInclusive / function->fCalled,
			function->fTimeExclusive,
			function->fTimeExclusive / function->fCalled, function->fFunction);
	}
}


Profiler::FunctionData*
Profiler::_FindFunction(const char* function)
{
	for (uint32 i = 0; i < kMaxFunctionEntries; i++) {
		if (fFunctionData[i].fFunction == NULL)
			break;
		if (!strcmp(fFunctionData[i].fFunction, function))
			return fFunctionData + i;
	}

	SpinLocker _(fFunctionLock);
	for (uint32 i = 0; i < kMaxFunctionEntries; i++) {
		if (fFunctionData[i].fFunction == NULL) {
			fFunctionData[i].fFunction = function;
			return fFunctionData + i;
		}
		if (!strcmp(fFunctionData[i].fFunction, function))
			return fFunctionData + i;
	}

	return NULL;
}


template<typename Type, Type Profiler::FunctionData::*Member>
/* static */ int
Profiler::_CompareFunctions(const void* _a, const void* _b)
{
	const FunctionData* a = static_cast<const FunctionData*>(_a);
	const FunctionData* b = static_cast<const FunctionData*>(_b);

	if (b->*Member > a->*Member)
		return 1;
	if (b->*Member < a->*Member)
		return -1;
	return 0;
}


template<typename Type, Type Profiler::FunctionData::*Member>
/* static */ int
Profiler::_CompareFunctionsPerCall(const void* _a, const void* _b)
{
	const FunctionData* a = static_cast<const FunctionData*>(_a);
	const FunctionData* b = static_cast<const FunctionData*>(_b);

	Type valueA = a->*Member / a->fCalled;
	Type valueB = b->*Member / b->fCalled;

	if (valueB > valueA)
		return 1;
	if (valueB < valueA)
		return -1;
	return 0;
}


static int
dump_profiler(int argc, char** argv)
{
	if (argc < 2) {
		Profiler::Get()->DumpCalled(0);
		return 0;
	}

	int32 count = 0;
	if (argc >= 3)
		count = parse_expression(argv[2]);
	count = std::max(count, int32(0));

	if (!strcmp(argv[1], "called"))
		Profiler::Get()->DumpCalled(count);
	else if (!strcmp(argv[1], "time-inclusive"))
		Profiler::Get()->DumpTimeInclusive(count);
	else if (!strcmp(argv[1], "time-inclusive-per-call"))
		Profiler::Get()->DumpTimeInclusivePerCall(count);
	else if (!strcmp(argv[1], "time-exclusive"))
		Profiler::Get()->DumpTimeExclusive(count);
	else if (!strcmp(argv[1], "time-exclusive-per-call"))
		Profiler::Get()->DumpTimeExclusivePerCall(count);
	else
		print_debugger_command_usage(argv[0]);

	return 0;
}


#endif	// SCHEDULER_PROFILING

