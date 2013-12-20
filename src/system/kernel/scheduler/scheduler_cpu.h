/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_CPU_H
#define KERNEL_SCHEDULER_CPU_H


#include <OS.h>

#include <thread.h>
#include <util/MinMaxHeap.h>

#include <cpufreq.h>

#include "RunQueue.h"
#include "scheduler_common.h"
#include "scheduler_modes.h"


namespace Scheduler {


struct ThreadData;

struct CPUEntry;
struct CoreEntry;
struct PackageEntry;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
class ThreadRunQueue : public RunQueue<ThreadData, THREAD_MAX_SET_PRIORITY> {
public:
						void			Dump() const;
};

struct CPUEntry : public MinMaxHeapLinkImpl<CPUEntry, int32> {
										CPUEntry();

						void			UpdatePriority(int32 priority);

						void			ComputeLoad();

						ThreadData*		ChooseNextThread(ThreadData* oldThread,
											bool putAtBack);

						void			TrackActivity(ThreadData* oldThreadData,
											ThreadData* nextThreadData);

						int32			fCPUNumber;
						CoreEntry*		fCore;

						rw_spinlock 	fSchedulerModeLock;

						ThreadRunQueue	fRunQueue;

						int32			fLoad;

						bigtime_t		fMeasureActiveTime;
						bigtime_t		fMeasureTime;

private:
	inline				void			_RequestPerformanceLevel(
											ThreadData* threadData);

} CACHE_LINE_ALIGN;

class CPUPriorityHeap : public MinMaxHeap<CPUEntry, int32> {
public:
										CPUPriorityHeap() { }
										CPUPriorityHeap(int32 cpuCount);

						void			Dump();
};

struct CoreEntry : public MinMaxHeapLinkImpl<CoreEntry, int32>,
	DoublyLinkedListLinkImpl<CoreEntry> {
										CoreEntry();

	inline				int32			GetLoad() const;
						void			UpdateLoad();

	static inline		CoreEntry*		GetCore(int32 cpu);

						int32			fCoreID;
						PackageEntry*	fPackage;

						int32			fCPUCount;
						CPUPriorityHeap	fCPUHeap;
						spinlock		fCPULock;

						int32			fStarvationCounter;
						DoublyLinkedList<ThreadData>	fThreadList;

						int32			fThreadCount;
						ThreadRunQueue	fRunQueue;
						spinlock		fQueueLock;

						bigtime_t		fActiveTime;
						seqlock			fActiveTimeLock;

						int32			fLoad;
						bool			fHighLoad;
} CACHE_LINE_ALIGN;

class CoreLoadHeap : public MinMaxHeap<CoreEntry, int32> {
public:
										CoreLoadHeap() { }
										CoreLoadHeap(int32 coreCount);

						void			Dump();
};

// gPackageEntries are used to decide which core should be woken up from the
// idle state. When aiming for performance we should use as many packages as
// possible with as little cores active in each package as possible (so that the
// package can enter any boost mode if it has one and the active core have more
// of the shared cache for themselves. If power saving is the main priority we
// should keep active cores on as little packages as possible (so that other
// packages can go to the deep state of sleep). The heap stores only packages
// with at least one core active and one core idle. The packages with all cores
// idle are stored in gPackageIdleList (in LIFO manner).
struct PackageEntry : public DoublyLinkedListLinkImpl<PackageEntry> {
											PackageEntry();

						int32				fPackageID;

						DoublyLinkedList<CoreEntry>	fIdleCores;
						int32				fIdleCoreCount;
						int32				fCoreCount;
						rw_spinlock			fCoreLock;
} CACHE_LINE_ALIGN;
typedef DoublyLinkedList<PackageEntry> IdlePackageList;

extern CPUEntry* gCPUEntries;

extern CoreEntry* gCoreEntries;
extern CoreLoadHeap gCoreLoadHeap;
extern CoreLoadHeap gCoreHighLoadHeap;
extern rw_spinlock gCoreHeapsLock;
extern int32 gCoreCount;

extern PackageEntry* gPackageEntries;
extern IdlePackageList gIdlePackageList;
extern rw_spinlock gIdlePackageLock;
extern int32 gPackageCount;


inline int32
CoreEntry::GetLoad() const
{
	ASSERT(fCPUCount >= 0);
	return fLoad / fCPUCount;
}


/* static */ inline CoreEntry*
CoreEntry::GetCore(int32 cpu)
{
	return gCPUEntries[cpu].fCore;
}


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_CPU_H

