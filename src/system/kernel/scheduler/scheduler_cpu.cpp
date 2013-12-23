/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include "scheduler_cpu.h"

#include <util/AutoLock.h>

#include <algorithm>

#include "scheduler_thread.h"


using namespace Scheduler;


class Scheduler::DebugDumper {
public:
	static	void		DumpCPURunQueue(CPUEntry* cpu);
	static	void		DumpCoreRunQueue(CoreEntry* core);
	static	void		DumpIdleCoresInPackage(PackageEntry* package);

};


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
	fLoad(0),
	fMeasureActiveTime(0),
	fMeasureTime(0)
{
	B_INITIALIZE_RW_SPINLOCK(&fSchedulerModeLock);
}


void
CPUEntry::Init(int32 id, CoreEntry* core)
{
	fCPUNumber = id;
	fCore = core;
}


void
CPUEntry::Start()
{
	fLoad = 0;
	fCore->AddCPU(this);
}


void
CPUEntry::Stop()
{
	cpu_ent* entry = &gCPU[fCPUNumber];

	// get rid of irqs
	SpinLocker locker(entry->irqs_lock);
	irq_assignment* irq
		= (irq_assignment*)list_get_first_item(&entry->irqs);
	while (irq != NULL) {
		locker.Unlock();

		assign_io_interrupt_to_cpu(irq->irq, -1);

		locker.Lock();
		irq = (irq_assignment*)list_get_first_item(&entry->irqs);
	}
	locker.Unlock();
}


void
CPUEntry::PushFront(ThreadData* thread, int32 priority)
{
	fRunQueue.PushFront(thread, priority);
}


void
CPUEntry::PushBack(ThreadData* thread, int32 priority)
{
	fRunQueue.PushBack(thread, priority);
}


void
CPUEntry::Remove(ThreadData* thread)
{
	fRunQueue.Remove(thread);
}


inline ThreadData*
CPUEntry::PeekThread() const
{
	return fRunQueue.PeekMaximum();
}


ThreadData*
CPUEntry::PeekIdleThread() const
{
	return fRunQueue.GetHead(B_IDLE_PRIORITY);
}


void
CPUEntry::UpdatePriority(int32 priority)
{
	if (gCPU[fCPUNumber].disabled)
		return;

	CPUPriorityHeap* cpuHeap = fCore->CPUHeap();
	int32 corePriority = CPUPriorityHeap::GetKey(cpuHeap->PeekMaximum());
	cpuHeap->ModifyKey(this, priority);

	if (gSingleCore)
		return;

	int32 maxPriority = CPUPriorityHeap::GetKey(cpuHeap->PeekMaximum());
	if (corePriority == maxPriority)
		return;

	PackageEntry* packageEntry = fCore->Package();
	if (maxPriority == B_IDLE_PRIORITY)
		packageEntry->CoreGoesIdle(fCore);
	else if (corePriority == B_IDLE_PRIORITY)
		packageEntry->CoreWakesUp(fCore);
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
		fCore->UpdateLoad(delta);
	}

	if (fLoad > kVeryHighLoad)
		gCurrentMode->rebalance_irqs(false);
}


ThreadData*
CPUEntry::ChooseNextThread(ThreadData* oldThread, bool putAtBack)
{
	CoreRunQueueLocker _(fCore);

	ThreadData* sharedThread = fCore->PeekThread();
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

		fCore->Remove(sharedThread, sharedThread->fWentSleepCount == 0);
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
		int32 cpu = entry->ID();
		int32 key = GetKey(entry);
		kprintf("%3" B_PRId32 " %8" B_PRId32 " %3" B_PRId32 "%%\n", cpu, key,
			entry->GetLoad() / 10);

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
	B_INITIALIZE_SEQLOCK(&fActiveTimeLock);
}


void
CoreEntry::Init(int32 id, PackageEntry* package)
{
	fCoreID = id;
	fPackage = package;
}


void
CoreEntry::PushFront(ThreadData* thread, int32 priority)
{
	fRunQueue.PushFront(thread, priority);
	atomic_add(&fThreadCount, 1);
}


void
CoreEntry::PushBack(ThreadData* thread, int32 priority)
{
	fRunQueue.PushBack(thread, priority);
	fThreadList.Insert(thread);

	atomic_add(&fThreadCount, 1);
}


void
CoreEntry::Remove(ThreadData* thread, bool starving)
{
	if (thread_is_idle_thread(thread->GetThread())
		|| fThreadList.Head() == thread) {
		atomic_add(&fStarvationCounter, 1);
	}
	if (starving)
		fThreadList.Remove(thread);
	fRunQueue.Remove(thread);
	atomic_add(&fThreadCount, -1);
}


inline ThreadData*
CoreEntry::PeekThread() const
{
	return fRunQueue.PeekMaximum();
}


void
CoreEntry::UpdateLoad(int32 delta)
{
	if (fCPUCount == 0) {
		fLoad = 0;
		return;
	}

	atomic_add(&fLoad, delta);

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


void
CoreEntry::AddCPU(CPUEntry* cpu)
{
	ASSERT(fCPUCount >= 0);
	if (fCPUCount++ == 0) {
		// core has been reenabled
		fLoad = 0;
		fHighLoad = false;
		gCoreLoadHeap.Insert(this, 0);

		fPackage->AddIdleCore(this);
	}

	fCPUHeap.Insert(cpu, B_IDLE_PRIORITY);
}


void
CoreEntry::RemoveCPU(CPUEntry* cpu, ThreadProcessing& threadPostProcessing)
{
	ASSERT(fCPUCount > 0);
	if (--fCPUCount == 0) {
		// core has been disabled
		if (fHighLoad) {
			gCoreHighLoadHeap.ModifyKey(this, -1);
			ASSERT(gCoreHighLoadHeap.PeekMinimum() == this);
			gCoreHighLoadHeap.RemoveMinimum();
		} else {
			gCoreLoadHeap.ModifyKey(this, -1);
			ASSERT(gCoreLoadHeap.PeekMinimum() == this);
			gCoreLoadHeap.RemoveMinimum();
		}

		fPackage->RemoveIdleCore(this);

		// get rid of threads
		thread_map(CoreEntry::_UnassignThread, this);

		fThreadCount = 0;
		while (fRunQueue.PeekMaximum() != NULL) {
			ThreadData* threadData = fRunQueue.PeekMaximum();

			fRunQueue.Remove(threadData);
			threadData->fEnqueued = false;

			if (threadData->fWentSleepCount == 0)
				fThreadList.Remove(threadData);
			threadData->fWentSleepCount = -1;

			ASSERT(threadData->Core() == NULL);
			threadPostProcessing(threadData);
		}
	}

	fCPUHeap.ModifyKey(cpu, THREAD_MAX_SET_PRIORITY + 1);
	ASSERT(fCPUHeap.PeekMaximum() == cpu);
	fCPUHeap.RemoveMaximum();

	ASSERT(cpu->GetLoad() >= 0 && cpu->GetLoad() <= kMaxLoad);
	fLoad -= cpu->GetLoad();
	ASSERT(fLoad >= 0);
}


/* static */ void
CoreEntry::_UnassignThread(Thread* thread, void* data)
{
	CoreEntry* core = static_cast<CoreEntry*>(data);
	ThreadData* threadData = thread->scheduler_data;

	if (threadData->Core() == core && thread->pinned_to_cpu == 0)
		threadData->UnassignCore();
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
		kprintf("%4" B_PRId32 " %3" B_PRId32 "%%\n", entry->ID(),
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


void
PackageEntry::Init(int32 id)
{
	fPackageID = id;
}


inline void
PackageEntry::CoreGoesIdle(CoreEntry* core)
{
	WriteSpinLocker _(fCoreLock);

	ASSERT(fIdleCoreCount >= 0);
	ASSERT(fIdleCoreCount < fCoreCount);

	fIdleCoreCount++;
	fIdleCores.Add(core);

	if (fIdleCoreCount == fCoreCount) {
		// package goes idle
		WriteSpinLocker _(gIdlePackageLock);
		gIdlePackageList.Add(this);
	}
}


inline void
PackageEntry::CoreWakesUp(CoreEntry* core)
{
	WriteSpinLocker _(fCoreLock);

	ASSERT(fIdleCoreCount > 0);
	ASSERT(fIdleCoreCount <= fCoreCount);

	fIdleCoreCount--;
	fIdleCores.Remove(core);

	if (fIdleCoreCount + 1 == fCoreCount) {
		// package wakes up
		WriteSpinLocker _(gIdlePackageLock);
		gIdlePackageList.Remove(this);
	}
}


void
PackageEntry::AddIdleCore(CoreEntry* core)
{
	fCoreCount++;
	fIdleCoreCount++;
	fIdleCores.Add(core);

	if (fCoreCount == 1)
		gIdlePackageList.Add(this);
}


void
PackageEntry::RemoveIdleCore(CoreEntry* core)
{
	fIdleCores.Remove(core);
	fIdleCoreCount--;
	fCoreCount--;

	if (fCoreCount == 0)
		gIdlePackageList.Remove(this);
}


/* static */ void
DebugDumper::DumpCPURunQueue(CPUEntry* cpu)
{
	ThreadRunQueue::ConstIterator iterator = cpu->fRunQueue.GetConstIterator();

	if (iterator.HasNext()
		&& !thread_is_idle_thread(iterator.Next()->GetThread())) {
		kprintf("\nCPU %" B_PRId32 " run queue:\n", cpu->ID());
		cpu->fRunQueue.Dump();
	}
}


/* static */ void
DebugDumper::DumpCoreRunQueue(CoreEntry* core)
{
	core->fRunQueue.Dump();
}


/* static */ void
DebugDumper::DumpIdleCoresInPackage(PackageEntry* package)
{
	kprintf("%-7" B_PRId32 " ", package->fPackageID);

	DoublyLinkedList<CoreEntry>::ReverseIterator iterator
		= package->fIdleCores.GetReverseIterator();
	if (iterator.HasNext()) {
		while (iterator.HasNext()) {
			CoreEntry* coreEntry = iterator.Next();
			kprintf("%" B_PRId32 "%s", coreEntry->ID(),
				iterator.HasNext() ? ", " : "");
		}
	} else
		kprintf("-");
	kprintf("\n");
}


static int
dump_run_queue(int argc, char **argv)
{
	int32 cpuCount = smp_get_num_cpus();
	int32 coreCount = gCoreCount;

	for (int32 i = 0; i < coreCount; i++) {
		kprintf("%sCore %" B_PRId32 " run queue:\n", i > 0 ? "\n" : "", i);
		DebugDumper::DumpCoreRunQueue(&gCoreEntries[i]);
	}

	for (int32 i = 0; i < cpuCount; i++)
		DebugDumper::DumpCPURunQueue(&gCPUEntries[i]);

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
		if (gCoreEntries[i].CPUCount() < 2)
			continue;

		kprintf("\nCore %" B_PRId32 " heap:\n", i);
		gCoreEntries[i].CPUHeap()->Dump();
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

		while (idleIterator.HasNext())
			DebugDumper::DumpIdleCoresInPackage(idleIterator.Next());
	} else
		kprintf("No idle packages.\n");

	return 0;
}


void Scheduler::init_debug_commands()
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

