/*
 * Copyright 2013-2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2009, Rene Gollent, rene@gollent.com.
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! The thread scheduler */


#include <OS.h>

#include <AutoDeleter.h>
#include <cpu.h>
#include <debug.h>
#include <int.h>
#include <kernel.h>
#include <kscheduler.h>
#include <listeners.h>
#include <load_tracking.h>
#include <scheduler_defs.h>
#include <smp.h>
#include <timer.h>
#include <util/Random.h>

#include "scheduler_common.h"
#include "scheduler_cpu.h"
#include "scheduler_locking.h"
#include "scheduler_modes.h"
#include "scheduler_profiler.h"
#include "scheduler_thread.h"
#include "scheduler_tracing.h"


namespace Scheduler {


class ThreadEnqueuer : public ThreadProcessing {
public:
	void		operator()(ThreadData* thread);
};

scheduler_mode gCurrentModeID;
scheduler_mode_operations* gCurrentMode;

bool gSingleCore;
bool gTrackCoreLoad;
bool gTrackCPULoad;

}	// namespace Scheduler

using namespace Scheduler;


static bool sSchedulerEnabled;

SchedulerListenerList gSchedulerListeners;
spinlock gSchedulerListenersLock = B_SPINLOCK_INITIALIZER;

static scheduler_mode_operations* sSchedulerModes[] = {
	&gSchedulerLowLatencyMode,
	&gSchedulerPowerSavingMode,
};

// Since CPU IDs used internally by the kernel bear no relation to the actual
// CPU topology the following arrays are used to efficiently get the core
// and the package that CPU in question belongs to.
static int32* sCPUToCore;
static int32* sCPUToPackage;


static void enqueue(Thread* thread, bool newOne);


void
ThreadEnqueuer::operator()(ThreadData* thread)
{
	enqueue(thread->GetThread(), false);
}


void
scheduler_dump_thread_data(Thread* thread)
{
	thread->scheduler_data->Dump();
}


static void
enqueue(Thread* thread, bool newOne)
{
	SCHEDULER_ENTER_FUNCTION();

	ThreadData* threadData = thread->scheduler_data;

	int32 threadPriority = threadData->GetEffectivePriority();
	T(EnqueueThread(thread, threadPriority));

	CPUEntry* targetCPU = NULL;
	CoreEntry* targetCore = NULL;
	if (thread->pinned_to_cpu > 0) {
		ASSERT(thread->previous_cpu != NULL);
		ASSERT(threadData->Core() != NULL);
		targetCPU = &gCPUEntries[thread->previous_cpu->cpu_num];
	} else if (gSingleCore)
		targetCore = &gCoreEntries[0];
	else if (threadData->Core() != NULL
		&& (!newOne || !threadData->HasCacheExpired())) {
		targetCore = threadData->Rebalance();
	}

	bool rescheduleNeeded = threadData->ChooseCoreAndCPU(targetCore, targetCPU);

	TRACE("enqueueing thread %ld with priority %ld on CPU %ld (core %ld)\n",
		thread->id, threadPriority, targetCPU->fCPUNumber, targetCore->fCoreID);

	threadData->Enqueue();

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);

	int32 heapPriority = CPUPriorityHeap::GetKey(targetCPU);
	if (threadPriority > heapPriority
		|| (threadPriority == heapPriority && rescheduleNeeded)) {

		if (targetCPU->ID() == smp_get_current_cpu())
			gCPU[targetCPU->ID()].invoke_scheduler = true;
		else {
			smp_send_ici(targetCPU->ID(), SMP_MSG_RESCHEDULE, 0, 0, 0,
				NULL, SMP_MSG_FLAG_ASYNC);
		}
	}
}


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
void
scheduler_enqueue_in_run_queue(Thread *thread)
{
	ASSERT(!are_interrupts_enabled());
	SCHEDULER_ENTER_FUNCTION();

	SchedulerModeLocker _;

	TRACE("enqueueing new thread %ld with static priority %ld\n", thread->id,
		thread->priority);

	ThreadData* threadData = thread->scheduler_data;

	if (threadData->ShouldCancelPenalty())
		threadData->CancelPenalty();

	enqueue(thread, true);
}


/*!	Sets the priority of a thread.
*/
int32
scheduler_set_thread_priority(Thread *thread, int32 priority)
{
	ASSERT(are_interrupts_enabled());

	InterruptsSpinLocker _(thread->scheduler_lock);
	SchedulerModeLocker modeLocker;

	SCHEDULER_ENTER_FUNCTION();

	ThreadData* threadData = thread->scheduler_data;
	int32 oldPriority = thread->priority;

	TRACE("changing thread %ld priority to %ld (old: %ld, effective: %ld)\n",
		thread->id, priority, oldPriority, threadData->GetEffectivePriority());

	thread->priority = priority;
	threadData->CancelPenalty();

	if (priority == oldPriority)
		return oldPriority;

	if (thread->state != B_THREAD_READY) {
		if (thread->state == B_THREAD_RUNNING) {
			ASSERT(threadData->Core() != NULL);

			ASSERT(thread->cpu != NULL);
			CPUEntry* cpu = &gCPUEntries[thread->cpu->cpu_num];

			CoreCPUHeapLocker _(threadData->Core());
			cpu->UpdatePriority(priority);
		}

		return oldPriority;
	}

	// The thread is in the run queue. We need to remove it and re-insert it at
	// a new position.

	T(RemoveThread(thread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadRemovedFromRunQueue,
		thread);

	if (threadData->Dequeue())
		enqueue(thread, true);

	return oldPriority;
}


void
scheduler_reschedule_ici()
{
	// This function is called as a result of an incoming ICI.
	// Make sure the reschedule() is invoked.
	get_cpu_struct()->invoke_scheduler = true;
}


static inline void
stop_cpu_timers(Thread* fromThread, Thread* toThread)
{
	SpinLocker teamLocker(&fromThread->team->time_lock);
	SpinLocker threadLocker(&fromThread->time_lock);

	if (fromThread->HasActiveCPUTimeUserTimers()
		|| fromThread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_stop_cpu_timers(fromThread, toThread);
	}
}


static inline void
continue_cpu_timers(Thread* thread, cpu_ent* cpu)
{
	SpinLocker teamLocker(&thread->team->time_lock);
	SpinLocker threadLocker(&thread->time_lock);

	if (thread->HasActiveCPUTimeUserTimers()
		|| thread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_continue_cpu_timers(thread, cpu->previous_thread);
	}
}


static void
thread_resumes(Thread* thread)
{
	cpu_ent* cpu = thread->cpu;

	release_spinlock(&cpu->previous_thread->scheduler_lock);

	// continue CPU time based user timers
	continue_cpu_timers(thread, cpu);

	// notify the user debugger code
	if ((thread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(thread);
}


void
scheduler_new_thread_entry(Thread* thread)
{
	thread_resumes(thread);

	SpinLocker locker(thread->time_lock);
	thread->last_time = system_time();
}


/*!	Switches the currently running thread.
	This is a service function for scheduler implementations.

	\param fromThread The currently running thread.
	\param toThread The thread to switch to. Must be different from
		\a fromThread.
*/
static inline void
switch_thread(Thread* fromThread, Thread* toThread)
{
	// notify the user debugger code
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_unscheduled(fromThread);

	// stop CPU time based user timers
	stop_cpu_timers(fromThread, toThread);

	// update CPU and Thread structures and perform the context switch
	cpu_ent* cpu = fromThread->cpu;
	toThread->previous_cpu = toThread->cpu = cpu;
	fromThread->cpu = NULL;
	cpu->running_thread = toThread;
	cpu->previous_thread = fromThread;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);

	// The use of fromThread below looks weird, but is correct. fromThread had
	// been unscheduled earlier, but is back now. For a thread scheduled the
	// first time the same is done in thread.cpp:common_thread_entry().
	thread_resumes(fromThread);
}


static void
reschedule(int32 nextState)
{
	ASSERT(!are_interrupts_enabled());
	SCHEDULER_ENTER_FUNCTION();

	int32 thisCPU = smp_get_current_cpu();

	CPUEntry* cpu = CPUEntry::GetCPU(thisCPU);
	CoreEntry* core = CoreEntry::GetCore(thisCPU);

	Thread* oldThread = thread_get_current_thread();
	ThreadData* oldThreadData = oldThread->scheduler_data;

	oldThreadData->StopCPUTime();

	SchedulerModeLocker modeLocker;

	TRACE("reschedule(): cpu %ld, current thread = %ld\n", thisCPU,
		oldThread->id);

	oldThread->state = nextState;

	// return time spent in interrupts
	oldThreadData->SetStolenInterruptTime(gCPU[thisCPU].interrupt_time);

	bool enqueueOldThread = false;
	bool putOldThreadAtBack = false;
	switch (nextState) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			enqueueOldThread = true;

			if (!oldThreadData->IsIdle()) {
				oldThreadData->Continues();
				if (oldThreadData->HasQuantumEnded(oldThread->cpu->preempted,
						oldThread->has_yielded)) {
					TRACE("enqueueing thread %ld into run queue priority ="
						" %ld\n", oldThread->id,
						oldThreadData->GetEffectivePriority());
					putOldThreadAtBack = true;
				} else {
					TRACE("putting thread %ld back in run queue priority ="
						" %ld\n", oldThread->id,
						oldThreadData->GetEffectivePriority());
					putOldThreadAtBack = false;
				}
			}

			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			oldThreadData->Dies();
			break;
		default:
			oldThreadData->GoesAway();
			TRACE("not enqueueing thread %ld into run queue next_state = %ld\n",
				oldThread->id, nextState);
			break;
	}

	oldThread->has_yielded = false;

	// select thread with the biggest priority and enqueue back the old thread
	ThreadData* nextThreadData;
	if (gCPU[thisCPU].disabled) {
		if (!oldThreadData->IsIdle()) {
			putOldThreadAtBack = oldThread->pinned_to_cpu == 0;
			oldThreadData->UnassignCore(true);

			CPURunQueueLocker cpuLocker(cpu);
			nextThreadData = cpu->PeekIdleThread();
			cpu->Remove(nextThreadData);
		} else
			nextThreadData = oldThreadData;
	} else {
		nextThreadData
			= cpu->ChooseNextThread(enqueueOldThread ? oldThreadData : NULL,
				putOldThreadAtBack);

		// update CPU heap
		CoreCPUHeapLocker cpuLocker(core);
		cpu->UpdatePriority(nextThreadData->GetEffectivePriority());
	}

	Thread* nextThread = nextThreadData->GetThread();
	ASSERT(!gCPU[thisCPU].disabled || nextThreadData->IsIdle());

	if (nextThread != oldThread) {
		if (enqueueOldThread) {
			if (putOldThreadAtBack)
				enqueue(oldThread, false);
			else
				oldThreadData->PutBack();
		}

		acquire_spinlock(&nextThread->scheduler_lock);
	}

	TRACE("reschedule(): cpu %ld, next thread = %ld\n", thisCPU,
		nextThread->id);

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled,
		oldThread, nextThread);

	ASSERT(nextThreadData->Core() == core);
	nextThread->state = B_THREAD_RUNNING;
	nextThreadData->StartCPUTime();

	// track CPU activity
	cpu->TrackActivity(oldThreadData, nextThreadData);

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		cpu->StartQuantumTimer(nextThreadData, oldThread->cpu->preempted);

		oldThread->cpu->preempted = false;
		if (!nextThreadData->IsIdle())
			nextThreadData->Continues();
		else
			gCurrentMode->rebalance_irqs(true);
		nextThreadData->StartQuantum();

		modeLocker.Unlock();

		SCHEDULER_EXIT_FUNCTION();

		if (nextThread != oldThread)
			switch_thread(oldThread, nextThread);
	}
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
void
scheduler_reschedule(int32 nextState)
{
	ASSERT(!are_interrupts_enabled());
	SCHEDULER_ENTER_FUNCTION();

	if (!sSchedulerEnabled) {
		Thread* thread = thread_get_current_thread();
		if (thread != NULL && nextState != B_THREAD_READY)
			panic("scheduler_reschedule_no_op() called in non-ready thread");
		return;
	}

	reschedule(nextState);
}


status_t
scheduler_on_thread_create(Thread* thread, bool idleThread)
{
	thread->scheduler_data = new(std::nothrow) ThreadData(thread);
	if (thread->scheduler_data == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


void
scheduler_on_thread_init(Thread* thread)
{
	ASSERT(thread->scheduler_data != NULL);

	if (thread_is_idle_thread(thread)) {
		static int32 sIdleThreadsID;
		int32 cpuID = atomic_add(&sIdleThreadsID, 1);

		thread->previous_cpu = &gCPU[cpuID];
		thread->pinned_to_cpu = 1;

		thread->scheduler_data->Init(CoreEntry::GetCore(cpuID));
	} else
		thread->scheduler_data->Init();
}


void
scheduler_on_thread_destroy(Thread* thread)
{
	delete thread->scheduler_data;
}


/*!	This starts the scheduler. Must be run in the context of the initial idle
	thread. Interrupts must be disabled and will be disabled when returning.
*/
void
scheduler_start()
{
	InterruptsSpinLocker _(thread_get_current_thread()->scheduler_lock);
	SCHEDULER_ENTER_FUNCTION();

	reschedule(B_THREAD_READY);
}


status_t
scheduler_set_operation_mode(scheduler_mode mode)
{
	if (mode != SCHEDULER_MODE_LOW_LATENCY
		&& mode != SCHEDULER_MODE_POWER_SAVING) {
		return B_BAD_VALUE;
	}

	dprintf("scheduler: switching to %s mode\n", sSchedulerModes[mode]->name);

	InterruptsBigSchedulerLocker _;

	gCurrentModeID = mode;
	gCurrentMode = sSchedulerModes[mode];
	gCurrentMode->switch_to_mode();

	ThreadData::ComputeQuantumLengths();

	return B_OK;
}


void
scheduler_set_cpu_enabled(int32 cpuID, bool enabled)
{
#if KDEBUG
	if (are_interrupts_enabled())
		panic("scheduler_set_cpu_enabled: called with interrupts enabled");
#endif

	dprintf("scheduler: %s CPU %" B_PRId32 "\n",
		enabled ? "enabling" : "disabling", cpuID);

	InterruptsBigSchedulerLocker _;

	gCurrentMode->set_cpu_enabled(cpuID, enabled);

	CPUEntry* cpu = &gCPUEntries[cpuID];
	CoreEntry* core = cpu->Core();

	ASSERT(core->CPUCount() >= 0);
	if (enabled)
		cpu->Start();
	else {
		cpu->UpdatePriority(B_IDLE_PRIORITY);

		ThreadEnqueuer enqueuer;
		core->RemoveCPU(cpu, enqueuer);
	}

	gCPU[cpuID].disabled = !enabled;

	if (!enabled) {
		cpu->Stop();

		// don't wait until the thread quantum ends
		if (smp_get_current_cpu() != cpuID) {
			smp_send_ici(cpuID, SMP_MSG_RESCHEDULE, 0, 0, 0, NULL,
				SMP_MSG_FLAG_ASYNC);
		}
	}
}


static void
traverse_topology_tree(const cpu_topology_node* node, int packageID, int coreID)
{
	switch (node->level) {
		case CPU_TOPOLOGY_SMT:
			sCPUToCore[node->id] = coreID;
			sCPUToPackage[node->id] = packageID;
			return;

		case CPU_TOPOLOGY_CORE:
			coreID = node->id;
			break;

		case CPU_TOPOLOGY_PACKAGE:
			packageID = node->id;
			break;

		default:
			break;
	}

	for (int32 i = 0; i < node->children_count; i++)
		traverse_topology_tree(node->children[i], packageID, coreID);
}


static status_t
build_topology_mappings(int32& cpuCount, int32& coreCount, int32& packageCount)
{
	cpuCount = smp_get_num_cpus();

	sCPUToCore = new(std::nothrow) int32[cpuCount];
	if (sCPUToCore == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToCoreDeleter(sCPUToCore);

	sCPUToPackage = new(std::nothrow) int32[cpuCount];
	if (sCPUToPackage == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToPackageDeleter(sCPUToPackage);

	coreCount = 0;
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0)
			coreCount++;
	}

	packageCount = 0;
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0
			&& gCPU[i].topology_id[CPU_TOPOLOGY_CORE] == 0) {
			packageCount++;
		}
	}

	const cpu_topology_node* root = get_cpu_topology();
	traverse_topology_tree(root, 0, 0);

	cpuToCoreDeleter.Detach();
	cpuToPackageDeleter.Detach();
	return B_OK;
}


static status_t
init()
{
	// create logical processor to core and package mappings
	int32 cpuCount, coreCount, packageCount;
	status_t result = build_topology_mappings(cpuCount, coreCount,
		packageCount);
	if (result != B_OK)
		return result;

	// disable parts of the scheduler logic that are not needed
	gSingleCore = coreCount == 1;
	scheduler_update_policy();

	gCoreCount = coreCount;
	gPackageCount = packageCount;

	gCPUEntries = new(std::nothrow) CPUEntry[cpuCount];
	if (gCPUEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUEntry> cpuEntriesDeleter(gCPUEntries);

	gCoreEntries = new(std::nothrow) CoreEntry[coreCount];
	if (gCoreEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CoreEntry> coreEntriesDeleter(gCoreEntries);

	gPackageEntries = new(std::nothrow) PackageEntry[packageCount];
	if (gPackageEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<PackageEntry> packageEntriesDeleter(gPackageEntries);

	new(&gCoreLoadHeap) CoreLoadHeap(coreCount);
	new(&gCoreHighLoadHeap) CoreLoadHeap(coreCount);

	new(&gIdlePackageList) IdlePackageList;

	for (int32 i = 0; i < cpuCount; i++) {
		CoreEntry* core = &gCoreEntries[sCPUToCore[i]];
		PackageEntry* package = &gPackageEntries[sCPUToPackage[i]];

		package->Init(sCPUToPackage[i]);
		core->Init(sCPUToCore[i], package);
		gCPUEntries[i].Init(i, core);

		core->AddCPU(&gCPUEntries[i]);
	}

	packageEntriesDeleter.Detach();
	coreEntriesDeleter.Detach();
	cpuEntriesDeleter.Detach();

	return B_OK;
}


void
scheduler_init()
{
	int32 cpuCount = smp_get_num_cpus();
	dprintf("scheduler_init: found %" B_PRId32 " logical cpu%s and %" B_PRId32
		" cache level%s\n", cpuCount, cpuCount != 1 ? "s" : "",
		gCPUCacheLevelCount, gCPUCacheLevelCount != 1 ? "s" : "");

#ifdef SCHEDULER_PROFILING
	Profiling::Profiler::Initialize();
#endif

	status_t result = init();
	if (result != B_OK)
		panic("scheduler_init: failed to initialize scheduler\n");

	scheduler_set_operation_mode(SCHEDULER_MODE_LOW_LATENCY);

	init_debug_commands();

#if SCHEDULER_TRACING
	add_debugger_command_etc("scheduler", &cmd_scheduler,
		"Analyze scheduler tracing information",
		"<thread>\n"
		"Analyzes scheduler tracing information for a given thread.\n"
		"  <thread>  - ID of the thread.\n", 0);
#endif
}


void
scheduler_enable_scheduling()
{
	sSchedulerEnabled = true;
}


void
scheduler_update_policy()
{
	gTrackCPULoad = increase_cpu_performance(0) == B_OK;
	gTrackCoreLoad = !gSingleCore || gTrackCPULoad;
	dprintf("scheduler switches: single core: %s, cpu load tracking: %s,"
		" core load tracking: %s\n", gSingleCore ? "true" : "false",
		gTrackCPULoad ? "true" : "false",
		gTrackCoreLoad ? "true" : "false");
}


// #pragma mark - SchedulerListener


SchedulerListener::~SchedulerListener()
{
}


// #pragma mark - kernel private


/*!	Add the given scheduler listener. Thread lock must be held.
*/
void
scheduler_add_listener(struct SchedulerListener* listener)
{
	InterruptsSpinLocker _(gSchedulerListenersLock);
	gSchedulerListeners.Add(listener);
}


/*!	Remove the given scheduler listener. Thread lock must be held.
*/
void
scheduler_remove_listener(struct SchedulerListener* listener)
{
	InterruptsSpinLocker _(gSchedulerListenersLock);
	gSchedulerListeners.Remove(listener);
}


// #pragma mark - Syscalls


bigtime_t
_user_estimate_max_scheduling_latency(thread_id id)
{
	syscall_64_bit_return_value();

	// get the thread
	Thread* thread;
	if (id < 0) {
		thread = thread_get_current_thread();
		thread->AcquireReference();
	} else {
		thread = Thread::Get(id);
		if (thread == NULL)
			return 0;
	}
	BReference<Thread> threadReference(thread, true);

#ifdef SCHEDULER_PROFILING
	InterruptsLocker _;
#endif

	ThreadData* threadData = thread->scheduler_data;
	CoreEntry* core = threadData->Core();
	if (core == NULL)
		core = &gCoreEntries[get_random<int32>() % gCoreCount];

	int32 threadCount = core->ThreadCount();
	if (core->CPUCount() > 0)
		threadCount /= core->CPUCount();

	if (threadData->GetEffectivePriority() > 0) {
		threadCount -= threadCount * THREAD_MAX_SET_PRIORITY
				/ threadData->GetEffectivePriority();
	}

	return std::min(std::max(threadCount * gCurrentMode->base_quantum,
			gCurrentMode->minimal_quantum),
		gCurrentMode->maximum_latency);
}


status_t
_user_set_scheduler_mode(int32 mode)
{
	scheduler_mode schedulerMode = static_cast<scheduler_mode>(mode);
	status_t error = scheduler_set_operation_mode(schedulerMode);
	if (error == B_OK)
		cpu_set_scheduler_mode(schedulerMode);
	return error;
}


int32
_user_get_scheduler_mode()
{
	return gCurrentModeID;
}

