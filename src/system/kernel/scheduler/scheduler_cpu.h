/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_CPU_H
#define KERNEL_SCHEDULER_CPU_H


#include <OS.h>

#include <thread.h>
#include <util/AutoLock.h>
#include <util/MinMaxHeap.h>

#include <cpufreq.h>

#include "RunQueue.h"
#include "scheduler_common.h"
#include "scheduler_modes.h"


namespace Scheduler {


class DebugDumper;

struct ThreadData;
class ThreadProcessing;

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

class CPUEntry : public MinMaxHeapLinkImpl<CPUEntry, int32> {
public:
										CPUEntry();

						void			Init(int32 id, CoreEntry* core);

	inline				int32			ID() const	{ return fCPUNumber; }
	inline				CoreEntry*		Core() const	{ return fCore; }

						void			Start();
						void			Stop();

	inline				void			EnterScheduler();
	inline				void			ExitScheduler();

	inline				void			LockScheduler();
	inline				void			UnlockScheduler();

						void			PushFront(ThreadData* thread,
											int32 priority);
						void			PushBack(ThreadData* thread,
											int32 priority);
						void			Remove(ThreadData* thread);
	inline				ThreadData*		PeekThread() const;
						ThreadData*		PeekIdleThread() const;

						void			UpdatePriority(int32 priority);

	inline				void			IncreaseActiveTime(
											bigtime_t activeTime);

	inline				int32			GetLoad() const	{ return fLoad; }
						void			ComputeLoad();

						ThreadData*		ChooseNextThread(ThreadData* oldThread,
											bool putAtBack);

						void			TrackActivity(ThreadData* oldThreadData,
											ThreadData* nextThreadData);

	static inline		CPUEntry*		GetCPU(int32 cpu);

private:
	inline				void			_RequestPerformanceLevel(
											ThreadData* threadData);

						int32			fCPUNumber;
						CoreEntry*		fCore;

						rw_spinlock 	fSchedulerModeLock;

						ThreadRunQueue	fRunQueue;

						int32			fLoad;

						bigtime_t		fMeasureActiveTime;
						bigtime_t		fMeasureTime;

						friend class DebugDumper;
} CACHE_LINE_ALIGN;

class CPUPriorityHeap : public MinMaxHeap<CPUEntry, int32> {
public:
										CPUPriorityHeap() { }
										CPUPriorityHeap(int32 cpuCount);

						void			Dump();
};

class CoreEntry : public MinMaxHeapLinkImpl<CoreEntry, int32>,
	public DoublyLinkedListLinkImpl<CoreEntry> {
public:
										CoreEntry();

						void			Init(int32 id, PackageEntry* package);

	inline				int32			ID() const	{ return fCoreID; }
	inline				PackageEntry*	Package() const	{ return fPackage; }
	inline				int32			CPUCount() const
											{ return fCPUCount; }

	inline				void			LockCPUHeap();
	inline				void			UnlockCPUHeap();

	inline				CPUPriorityHeap*	CPUHeap();

	inline				int32			ThreadCount() const
											{ return fThreadCount; }

	inline				void			LockRunQueue();
	inline				void			UnlockRunQueue();

						void			PushFront(ThreadData* thread,
											int32 priority);
						void			PushBack(ThreadData* thread,
											int32 priority);
						void			Remove(ThreadData* thread,
											bool starving);
	inline				ThreadData*		PeekThread() const;

	inline				bigtime_t		GetActiveTime() const;
	inline				void			IncreaseActiveTime(
											bigtime_t activeTime);

	inline				int32			GetLoad() const;
						void			UpdateLoad(int32 delta);

	inline				int32			StarvationCounter() const;

						void			AddCPU(CPUEntry* cpu);
						void			RemoveCPU(CPUEntry* cpu,
											ThreadProcessing&
												threadPostProcessing);

	static inline		CoreEntry*		GetCore(int32 cpu);

private:
	static				void			_UnassignThread(Thread* thread,
											void* core);

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
	mutable				seqlock			fActiveTimeLock;

						int32			fLoad;
						bool			fHighLoad;

						friend class DebugDumper;
} CACHE_LINE_ALIGN;

class CoreRunQueueLocking {
public:
	inline bool Lock(CoreEntry* core)
	{
		core->LockRunQueue();
		return true;
	}

	inline void Unlock(CoreEntry* core)
	{
		core->UnlockRunQueue();
	}
};

typedef AutoLocker<CoreEntry, CoreRunQueueLocking> CoreRunQueueLocker;

class CoreCPUHeapLocking {
public:
	inline bool Lock(CoreEntry* core)
	{
		core->LockCPUHeap();
		return true;
	}

	inline void Unlock(CoreEntry* core)
	{
		core->UnlockCPUHeap();
	}
};

typedef AutoLocker<CoreEntry, CoreCPUHeapLocking> CoreCPUHeapLocker;

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
class PackageEntry : public DoublyLinkedListLinkImpl<PackageEntry> {
public:
											PackageEntry();

						void				Init(int32 id);

	inline				void				CoreGoesIdle(CoreEntry* core);
	inline				void				CoreWakesUp(CoreEntry* core);

	inline				CoreEntry*			GetIdleCore() const;

						void				AddIdleCore(CoreEntry* core);
						void				RemoveIdleCore(CoreEntry* core);

	static inline		PackageEntry*		GetMostIdlePackage();
	static inline		PackageEntry*		GetLeastIdlePackage();

private:
						int32				fPackageID;

						DoublyLinkedList<CoreEntry>	fIdleCores;
						int32				fIdleCoreCount;
						int32				fCoreCount;
						rw_spinlock			fCoreLock;

						friend class DebugDumper;
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


inline void
CPUEntry::EnterScheduler()
{
	acquire_read_spinlock(&fSchedulerModeLock);
}


inline void
CPUEntry::ExitScheduler()
{
	release_read_spinlock(&fSchedulerModeLock);
}


inline void
CPUEntry::LockScheduler()
{
	acquire_write_spinlock(&fSchedulerModeLock);
}


inline void
CPUEntry::UnlockScheduler()
{
	release_write_spinlock(&fSchedulerModeLock);
}


inline void
CPUEntry::IncreaseActiveTime(bigtime_t activeTime)
{
	fMeasureActiveTime += activeTime;
}


/* static */ inline CPUEntry*
CPUEntry::GetCPU(int32 cpu)
{
	return &gCPUEntries[cpu];
}


inline void
CoreEntry::LockCPUHeap()
{
	acquire_spinlock(&fCPULock);
}


inline void
CoreEntry::UnlockCPUHeap()
{
	release_spinlock(&fCPULock);
}


inline CPUPriorityHeap*
CoreEntry::CPUHeap()
{
	return &fCPUHeap;
}


inline void
CoreEntry::LockRunQueue()
{
	acquire_spinlock(&fQueueLock);
}


inline void
CoreEntry::UnlockRunQueue()
{
	release_spinlock(&fQueueLock);
}


inline void
CoreEntry::IncreaseActiveTime(bigtime_t activeTime)
{
	WriteSequentialLocker _(fActiveTimeLock);
	fActiveTime += activeTime;
}


inline bigtime_t
CoreEntry::GetActiveTime() const
{
	bigtime_t activeTime;

	uint32 count;
	do {
		count = acquire_read_seqlock(&fActiveTimeLock);
		activeTime = fActiveTime;
	} while (!release_read_seqlock(&fActiveTimeLock, count));

	return activeTime;
}


inline int32
CoreEntry::GetLoad() const
{
	ASSERT(fCPUCount >= 0);
	return fLoad / fCPUCount;
}


inline int32
CoreEntry::StarvationCounter() const
{
	return fStarvationCounter;
}


/* static */ inline CoreEntry*
CoreEntry::GetCore(int32 cpu)
{
	return gCPUEntries[cpu].Core();
}


inline CoreEntry*
PackageEntry::GetIdleCore() const
{
	return fIdleCores.Last();
}


/* static */ inline PackageEntry*
PackageEntry::GetMostIdlePackage()
{
	PackageEntry* current = &gPackageEntries[0];
	for (int32 i = 1; i < gPackageCount; i++) {
		if (gPackageEntries[i].fIdleCoreCount > current->fIdleCoreCount)
			current = &gPackageEntries[i];
	}

	if (current->fIdleCoreCount == 0)
		return NULL;

	return current;
}


/* static */ inline PackageEntry*
PackageEntry::GetLeastIdlePackage()
{
	PackageEntry* package = NULL;

	for (int32 i = 0; i < gPackageCount; i++) {
		PackageEntry* current = &gPackageEntries[i];

		int32 currentIdleCoreCount = current->fIdleCoreCount;
		if (currentIdleCoreCount != 0 && (package == NULL
				|| currentIdleCoreCount < package->fIdleCoreCount)) {
			package = current;
		}
	}

	return package;
}


}	// namespace Scheduler


#endif	// KERNEL_SCHEDULER_CPU_H

