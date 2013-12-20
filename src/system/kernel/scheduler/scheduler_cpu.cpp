/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include "scheduler_cpu.h"

#include <util/AutoLock.h>

#include <algorithm>

#include "scheduler_thread.h"


using namespace Scheduler;


static CPUPriorityHeap sDebugCPUHeap;
static CoreLoadHeap sDebugCoreHeap;


void
ThreadRunQueue::Dump() const
{
	ThreadRunQueue::ConstIterator iterator = GetConstIterator();
	if (!iterator.HasNext())
		kprintf("Run queue is empty.\n");
	else {
		kprintf("thread      id      priority penalty  name\n");
		while (iterator.HasNext()) {
			ThreadData* threadData = iterator.Next();
			Thread* thread = threadData->GetThread();

			kprintf("%p  %-7" B_PRId32 " %-8" B_PRId32 " %-8" B_PRId32 " %s\n",
				thread, thread->id, thread->priority,
				threadData->GetEffectivePriority(), thread->name);
		}
	}
}


CPUEntry::CPUEntry()
	:
	fPriority(B_IDLE_PRIORITY),
	fLoad(0),
	fMeasureActiveTime(0),
	fMeasureTime(0)
{
	B_INITIALIZE_RW_SPINLOCK(&fSchedulerModeLock);
}


void
CPUEntry::UpdatePriority(int32 priority)
{
	int32 corePriority = CPUPriorityHeap::GetKey(fCore->fCPUHeap.PeekMaximum());
	fCore->fCPUHeap.ModifyKey(this, priority);

	if (gSingleCore)
		return;

	int32 maxPriority = CPUPriorityHeap::GetKey(fCore->fCPUHeap.PeekMaximum());
	if (corePriority == maxPriority)
		return;

	PackageEntry* packageEntry = fCore->fPackage;
	if (maxPriority == B_IDLE_PRIORITY) {
		WriteSpinLocker _(packageEntry->fCoreLock);

		// core goes idle
		ASSERT(packageEntry->fIdleCoreCount >= 0);
		ASSERT(packageEntry->fIdleCoreCount < packageEntry->fCoreCount);

		packageEntry->fIdleCoreCount++;
		packageEntry->fIdleCores.Add(fCore);

		if (packageEntry->fIdleCoreCount == packageEntry->fCoreCount) {
			// package goes idle
			WriteSpinLocker _(gIdlePackageLock);
			gIdlePackageList.Add(packageEntry);
		}
	} else if (corePriority == B_IDLE_PRIORITY) {
		WriteSpinLocker _(packageEntry->fCoreLock);

		// core wakes up
		ASSERT(packageEntry->fIdleCoreCount > 0);
		ASSERT(packageEntry->fIdleCoreCount <= packageEntry->fCoreCount);

		packageEntry->fIdleCoreCount--;
		packageEntry->fIdleCores.Remove(fCore);

		if (packageEntry->fIdleCoreCount + 1 == packageEntry->fCoreCount) {
			// package wakes up
			WriteSpinLocker _(gIdlePackageLock);
			gIdlePackageList.Remove(packageEntry);
		}
	}
}


void
CPUEntry::ComputeLoad()
{
	ASSERT(!gSingleCore);
	ASSERT(fCPUNumber == smp_get_current_cpu());

	int oldLoad = compute_load(fMeasureTime, fMeasureActiveTime, fLoad);
	if (oldLoad < 0)
		return;

	if (oldLoad != fLoad) {
		int32 delta = fLoad - oldLoad;
		atomic_add(&fCore->fLoad, delta);

		fCore->UpdateLoad();
	}

	if (fLoad > kVeryHighLoad)
		gCurrentMode->rebalance_irqs(false);
}


ThreadData*
CPUEntry::ChooseNextThread(ThreadData* oldThread, bool putAtBack)
{
	SpinLocker runQueueLocker(fCore->fQueueLock);

	ThreadData* sharedThread = fCore->fRunQueue.PeekMaximum();
	ThreadData* pinnedThread = fRunQueue.PeekMaximum();

	ASSERT(sharedThread != NULL || pinnedThread != NULL || oldThread != NULL);

	int32 pinnedPriority = -1;
	if (pinnedThread != NULL)
		pinnedPriority = pinnedThread->GetEffectivePriority();

	int32 sharedPriority = -1;
	if (sharedThread != NULL)
		sharedPriority = sharedThread->GetEffectivePriority();

	int32 oldPriority = -1;
	if (oldThread != NULL)
		oldPriority = oldThread->GetEffectivePriority();

	int32 rest = std::max(pinnedPriority, sharedPriority);
	if (oldPriority > rest || (!putAtBack && oldPriority == rest))
		return oldThread;

	if (sharedPriority > pinnedPriority) {
		sharedThread->fEnqueued = false;

		fCore->fRunQueue.Remove(sharedThread);
		if (thread_is_idle_thread(sharedThread->GetThread())
			|| fCore->fThreadList.Head() == sharedThread) {
			atomic_add(&fCore->fStarvationCounter, 1);
		}

		if (sharedThread->fWentSleepCount == 0)
			fCore->fThreadList.Remove(sharedThread);

		atomic_add(&fCore->fThreadCount, -1);
		return sharedThread;
	}

	pinnedThread->fEnqueued = false;
	fRunQueue.Remove(pinnedThread);
	return pinnedThread;
}


void
CPUEntry::TrackActivity(ThreadData* oldThreadData, ThreadData* nextThreadData)
{
	cpu_ent* cpuEntry = &gCPU[fCPUNumber];

	Thread* oldThread = oldThreadData->GetThread();
	if (!thread_is_idle_thread(oldThread)) {
		bigtime_t active
			= (oldThread->kernel_time - cpuEntry->last_kernel_time)
				+ (oldThread->user_time - cpuEntry->last_user_time);

		WriteSequentialLocker locker(cpuEntry->active_time_lock);
		cpuEntry->active_time += active;
		locker.Unlock();

		oldThreadData->UpdateActivity(active);
	}

	oldThreadData->ComputeLoad();
	nextThreadData->ComputeLoad();
	if (!gSingleCore && !cpuEntry->disabled)
		ComputeLoad();

	Thread* nextThread = nextThreadData->GetThread();
	if (!thread_is_idle_thread(nextThread)) {
		cpuEntry->last_kernel_time = nextThread->kernel_time;
		cpuEntry->last_user_time = nextThread->user_time;

		nextThreadData->fLastInterruptTime = cpuEntry->interrupt_time;

		_RequestPerformanceLevel(nextThreadData);
	}
}


inline void
CPUEntry::_RequestPerformanceLevel(ThreadData* threadData)
{
	int32 load = std::max(threadData->GetLoad(), fCore->GetLoad());
	load = std::min(std::max(load, int32(0)), kMaxLoad);

	if (load < kTargetLoad) {
		int32 delta = kTargetLoad - load;

		delta *= kTargetLoad;
		delta /= kCPUPerformanceScaleMax;

		decrease_cpu_performance(delta);
	} else {
		int32 delta = load - kTargetLoad;
		delta *= kMaxLoad - kTargetLoad;
		delta /= kCPUPerformanceScaleMax;

		increase_cpu_performance(delta);
	}
}


CPUPriorityHeap::CPUPriorityHeap(int32 cpuCount)
	:
	MinMaxHeap<CPUEntry, int32>(cpuCount)
{
}


void
CPUPriorityHeap::Dump()
{
	kprintf("cpu priority load\n");
	CPUEntry* entry = PeekMinimum();
	while (entry) {
		int32 cpu = entry->fCPUNumber;
		int32 key = GetKey(entry);
		kprintf("%3" B_PRId32 " %8" B_PRId32 " %3" B_PRId32 "%%\n", cpu, key,
			entry->fLoad / 10);

		RemoveMinimum();
		sDebugCPUHeap.Insert(entry, key);

		entry = PeekMinimum();
	}

	entry = sDebugCPUHeap.PeekMinimum();
	while (entry) {
		int32 key = GetKey(entry);
		sDebugCPUHeap.RemoveMinimum();
		Insert(entry, key);
		entry = sDebugCPUHeap.PeekMinimum();
	}
}


CoreEntry::CoreEntry()
	:
	fCPUCount(0),
	fStarvationCounter(0),
	fThreadCount(0),
	fActiveTime(0),
	fLoad(0),
	fHighLoad(false)
{
	B_INITIALIZE_SPINLOCK(&fCPULock);
	B_INITIALIZE_SPINLOCK(&fQueueLock);
}


void
CoreEntry::UpdateLoad()
{
	ASSERT(!gSingleCore);

	if (fCPUCount == 0) {
		fLoad = 0;
		return;
	}

	WriteSpinLocker coreLocker(gCoreHeapsLock);

	int32 newKey = GetLoad();
	int32 oldKey = CoreLoadHeap::GetKey(this);

	ASSERT(oldKey >= 0 && oldKey <= kMaxLoad);
	ASSERT(newKey >= 0 && newKey <= kMaxLoad);

	if (oldKey == newKey)
		return;

	if (newKey > kHighLoad) {
		if (!fHighLoad) {
			gCoreLoadHeap.ModifyKey(this, -1);
			ASSERT(gCoreLoadHeap.PeekMinimum() == this);
			gCoreLoadHeap.RemoveMinimum();

			gCoreHighLoadHeap.Insert(this, newKey);

			fHighLoad = true;
		} else
			gCoreHighLoadHeap.ModifyKey(this, newKey);
	} else if (newKey < kMediumLoad) {
		if (fHighLoad) {
			gCoreHighLoadHeap.ModifyKey(this, -1);
			ASSERT(gCoreHighLoadHeap.PeekMinimum() == this);
			gCoreHighLoadHeap.RemoveMinimum();

			gCoreLoadHeap.Insert(this, newKey);

			fHighLoad = false;
		} else
			gCoreLoadHeap.ModifyKey(this, newKey);
	} else {
		if (fHighLoad)
			gCoreHighLoadHeap.ModifyKey(this, newKey);
		else
			gCoreLoadHeap.ModifyKey(this, newKey);
	}
}


CoreLoadHeap::CoreLoadHeap(int32 coreCount)
	:
	MinMaxHeap<CoreEntry, int32>(coreCount)
{
}


void
CoreLoadHeap::Dump()
{
	CoreEntry* entry = PeekMinimum();
	while (entry) {
		int32 key = GetKey(entry);
		kprintf("%4" B_PRId32 " %3" B_PRId32 "%%\n", entry->fCoreID,
			entry->GetLoad() / 10);

		RemoveMinimum();
		sDebugCoreHeap.Insert(entry, key);

		entry = PeekMinimum();
	}

	entry = sDebugCoreHeap.PeekMinimum();
	while (entry) {
		int32 key = GetKey(entry);
		sDebugCoreHeap.RemoveMinimum();
		Insert(entry, key);
		entry = sDebugCoreHeap.PeekMinimum();
	}
}


PackageEntry::PackageEntry()
	:
	fIdleCoreCount(0),
	fCoreCount(0)
{
	B_INITIALIZE_RW_SPINLOCK(&fCoreLock);
}


static int
dump_run_queue(int argc, char **argv)
{
	int32 cpuCount = smp_get_num_cpus();
	int32 coreCount = gCoreCount;

	
	for (int32 i = 0; i < coreCount; i++) {
		kprintf("%sCore %" B_PRId32 " run queue:\n", i > 0 ? "\n" : "", i);
		gCoreEntries[i].fRunQueue.Dump();
	}

	for (int32 i = 0; i < cpuCount; i++) {
		CPUEntry* cpu = &gCPUEntries[i];
		ThreadRunQueue::ConstIterator iterator
			= cpu->fRunQueue.GetConstIterator();

		if (iterator.HasNext()
			&& !thread_is_idle_thread(iterator.Next()->GetThread())) {
			kprintf("\nCPU %" B_PRId32 " run queue:\n", i);
			cpu->fRunQueue.Dump();
		}
	}

	return 0;
}


static int
dump_cpu_heap(int argc, char** argv)
{
	kprintf("core load\n");
	gCoreLoadHeap.Dump();
	kprintf("\n");
	gCoreHighLoadHeap.Dump();

	for (int32 i = 0; i < gCoreCount; i++) {
		if (gCoreEntries[i].fCPUCount < 2)
			continue;

		kprintf("\nCore %" B_PRId32 " heap:\n", i);
		gCoreEntries[i].fCPUHeap.Dump();
	}

	return 0;
}


static int
dump_idle_cores(int argc, char** argv)
{
	kprintf("Idle packages:\n");
	IdlePackageList::ReverseIterator idleIterator
		= gIdlePackageList.GetReverseIterator();

	if (idleIterator.HasNext()) {
		kprintf("package cores\n");

		while (idleIterator.HasNext()) {
			PackageEntry* entry = idleIterator.Next();
			kprintf("%-7" B_PRId32 " ", entry->fPackageID);

			DoublyLinkedList<CoreEntry>::ReverseIterator iterator
				= entry->fIdleCores.GetReverseIterator();
			if (iterator.HasNext()) {
				while (iterator.HasNext()) {
					CoreEntry* coreEntry = iterator.Next();
					kprintf("%" B_PRId32 "%s", coreEntry->fCoreID,
						iterator.HasNext() ? ", " : "");
				}
			} else
				kprintf("-");
			kprintf("\n");
		}
	} else
		kprintf("No idle packages.\n");

	return 0;
}


void Scheduler::init_debug_commands(void)
{
	new(&sDebugCPUHeap) CPUPriorityHeap(smp_get_num_cpus());
	new(&sDebugCoreHeap) CoreLoadHeap(smp_get_num_cpus());

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
	if (!gSingleCore) {
		add_debugger_command_etc("cpu_heap", &dump_cpu_heap,
			"List CPUs in CPU priority heap",
			"\nList CPUs in CPU priority heap", 0);
		add_debugger_command_etc("idle_cores", &dump_idle_cores,
			"List idle cores", "\nList idle cores", 0);
	}
}

