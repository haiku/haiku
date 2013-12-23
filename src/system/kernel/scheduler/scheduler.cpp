/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
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
#include "scheduler_modes.h"
#include "scheduler_thread.h"
#include "scheduler_tracing.h"


namespace Scheduler {


class SchedulerModeLocker : public ReadSpinLocker {
public:
	SchedulerModeLocker(bool alreadyLocked = false, bool lockIfNotLocked = true)
		:
		ReadSpinLocker(gCPUEntries[smp_get_current_cpu()].fSchedulerModeLock,
			alreadyLocked, lockIfNotLocked)
	{
	}
};

class InterruptsSchedulerModeLocker : public InterruptsReadSpinLocker {
public:
	InterruptsSchedulerModeLocker(bool alreadyLocked = false,
		bool lockIfNotLocked = true)
		:
		InterruptsReadSpinLocker(
			gCPUEntries[smp_get_current_cpu()].fSchedulerModeLock,
			alreadyLocked, lockIfNotLocked)
	{
	}
};

class InterruptsBigSchedulerLocking {
public:
	bool Lock(int* lockable)
	{
		*lockable = disable_interrupts();
		for (int32 i = 0; i < smp_get_num_cpus(); i++)
			acquire_write_spinlock(&gCPUEntries[i].fSchedulerModeLock);
		return true;
	}

	void Unlock(int* lockable)
	{
		for (int32 i = 0; i < smp_get_num_cpus(); i++)
			release_write_spinlock(&gCPUEntries[i].fSchedulerModeLock);
		restore_interrupts(*lockable);
	}
};

class InterruptsBigSchedulerLocker :
	public AutoLocker<int, InterruptsBigSchedulerLocking> {
public:
	InterruptsBigSchedulerLocker()
		:
		AutoLocker<int, InterruptsBigSchedulerLocking>(&fState, false, true)
	{
	}

private:
	int		fState;
};

class ThreadEnqueuer : public ThreadProcessing {
public:
	void		operator()(ThreadData* thread);
};

scheduler_mode gCurrentModeID;
scheduler_mode_operations* gCurrentMode;

bool gSingleCore;

CPUEntry* gCPUEntries;

CoreEntry* gCoreEntries;
CoreLoadHeap gCoreLoadHeap;
CoreLoadHeap gCoreHighLoadHeap;
rw_spinlock gCoreHeapsLock = B_RW_SPINLOCK_INITIALIZER;
int32 gCoreCount;

PackageEntry* gPackageEntries;
IdlePackageList gIdlePackageList;
rw_spinlock gIdlePackageLock = B_RW_SPINLOCK_INITIALIZER;
int32 gPackageCount;


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
	ASSERT(thread != NULL);

	ThreadData* threadData = thread->scheduler_data;

	int32 threadPriority = threadData->GetEffectivePriority();
	T(EnqueueThread(thread, threadPriority));

	CPUEntry* targetCPU = NULL;
	CoreEntry* targetCore = NULL;
	if (thread->pinned_to_cpu > 0) {
		ASSERT(thread->previous_cpu != NULL);
		targetCPU = &gCPUEntries[thread->previous_cpu->cpu_num];
	} else if (gSingleCore)
		targetCore = &gCoreEntries[0];
	else if (threadData->Core() != NULL
		&& (!newOne || !threadData->HasCacheExpired())
		&& !threadData->ShouldRebalance()) {
		targetCore = threadData->Core();
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

		if (targetCPU->fCPUNumber == smp_get_current_cpu())
			gCPU[targetCPU->fCPUNumber].invoke_scheduler = true;
		else {
			smp_send_ici(targetCPU->fCPUNumber, SMP_MSG_RESCHEDULE, 0, 0, 0,
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
#if KDEBUG
	if (are_interrupts_enabled())
		panic("scheduler_enqueue_in_run_queue: called with interrupts enabled");
#endif

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
#if KDEBUG
	if (!are_interrupts_enabled())
		panic("scheduler_set_thread_priority: called with interrupts disabled");
#endif

	InterruptsSpinLocker _(thread->scheduler_lock);
	SchedulerModeLocker modeLocker;

	ThreadData* threadData = thread->scheduler_data;
	int32 oldPriority = thread->priority;

	TRACE("changing thread %ld priority to %ld (old: %ld, effective: %ld)\n",
		thread->id, priority, oldPriority, threadData->GetEffectivePriority());

	threadData->CancelPenalty();

	if (priority == thread->priority)
		return thread->priority;
	thread->priority = priority;

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


static inline void
reschedule_needed()
{
	// This function is called as a result of either the timer event set by the
	// scheduler or an incoming ICI. Make sure the reschedule() is invoked.
	get_cpu_struct()->invoke_scheduler = true;
}


void
scheduler_reschedule_ici()
{
	reschedule_needed();
}


static int32
reschedule_event(timer* /* unused */)
{
	reschedule_needed();
	get_cpu_struct()->preempted = true;
	return B_HANDLED_INTERRUPT;
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


static inline void
update_thread_times(Thread* oldThread, Thread* nextThread)
{
	bigtime_t now = system_time();
	if (oldThread == nextThread) {
		SpinLocker _(oldThread->time_lock);
		oldThread->kernel_time += now - oldThread->last_time;
		oldThread->last_time = now;
	} else {
		SpinLocker locker(oldThread->time_lock);
		oldThread->kernel_time += now - oldThread->last_time;
		oldThread->last_time = 0;
		locker.Unlock();

		locker.SetTo(nextThread->time_lock, false);
		nextThread->last_time = now;
	}

	// If the old thread's team has user time timers, check them now.
	Team* team = oldThread->team;

	SpinLocker _(team->time_lock);
	if (team->HasActiveUserTimeUserTimers())
		user_timer_check_team_user_timers(team);
}


static void
reschedule(int32 nextState)
{
	ASSERT(!are_interrupts_enabled());

	SchedulerModeLocker modeLocker;

	Thread* oldThread = thread_get_current_thread();

	int32 thisCPU = smp_get_current_cpu();

	CPUEntry* cpu = &gCPUEntries[thisCPU];
	CoreEntry* core = CoreEntry::GetCore(thisCPU);

	TRACE("reschedule(): cpu %ld, current thread = %ld\n", thisCPU,
		oldThread->id);

	oldThread->state = nextState;
	ThreadData* oldThreadData = oldThread->scheduler_data;

	// return time spent in interrupts
	oldThreadData->fStolenTime
		+= gCPU[thisCPU].interrupt_time - oldThreadData->fLastInterruptTime;

	bool enqueueOldThread = false;
	bool putOldThreadAtBack = false;
	switch (nextState) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			enqueueOldThread = true;

			if (oldThreadData->HasQuantumEnded(oldThread->cpu->preempted,
					oldThread->has_yielded)) {
				oldThreadData->IncreasePenalty();

				TRACE("enqueueing thread %ld into run queue priority = %ld\n",
					oldThread->id, oldThreadData->GetEffectivePriority());
				putOldThreadAtBack = true;
			} else {
				TRACE("putting thread %ld back in run queue priority = %ld\n",
					oldThread->id, oldThreadData->GetEffectivePriority());
				putOldThreadAtBack = false;
			}

			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			break;
		default:
			oldThreadData->IncreasePenalty();
			oldThreadData->GoesAway();
			TRACE("not enqueueing thread %ld into run queue next_state = %ld\n",
				oldThread->id, nextState);
			break;
	}

	oldThread->has_yielded = false;

	// select thread with the biggest priority and enqueue back the old thread
	ThreadData* nextThreadData;
	if (gCPU[thisCPU].disabled) {
		if (!thread_is_idle_thread(oldThread)) {
			CoreRunQueueLocker _(core);

			nextThreadData = cpu->fRunQueue.GetHead(B_IDLE_PRIORITY);
		 	cpu->fRunQueue.Remove(nextThreadData);
			nextThreadData->fEnqueued = false;

			putOldThreadAtBack = oldThread->pinned_to_cpu == 0;
		} else
			nextThreadData = oldThreadData;
	} else {
		nextThreadData
			= cpu->ChooseNextThread(enqueueOldThread ? oldThreadData : NULL,
				putOldThreadAtBack);
	}

	Thread* nextThread = nextThreadData->GetThread();
	CoreCPUHeapLocker cpuLocker(core);
	cpu->UpdatePriority(nextThreadData->GetEffectivePriority());
	cpuLocker.Unlock();

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

	// update CPU heap
	cpuLocker.Lock();
	cpu->UpdatePriority(nextThreadData->GetEffectivePriority());
	cpuLocker.Unlock();

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	update_thread_times(oldThread, nextThread);

	// track CPU activity
	cpu->TrackActivity(oldThreadData, nextThreadData);

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		timer* quantumTimer = &oldThread->cpu->quantum_timer;
		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = false;
		if (!thread_is_idle_thread(nextThread)) {
			bigtime_t quantum = nextThreadData->ComputeQuantum();
			add_timer(quantumTimer, &reschedule_event, quantum,
				B_ONE_SHOT_RELATIVE_TIMER);
		} else {
			nextThreadData->fQuantumStart = system_time();

			gCurrentMode->rebalance_irqs(true);
		}

		modeLocker.Unlock();
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
#if KDEBUG
	if (are_interrupts_enabled())
		panic("scheduler_reschedule: called with interrupts enabled");
#endif

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
	CoreEntry* core = cpu->fCore;

	int32 oldCPUCount = core->CPUCount();
	ASSERT(oldCPUCount >= 0);
	if (enabled) {
		cpu->fLoad = 0;
		core->AddCPU(cpu);
	} else {
		cpu->UpdatePriority(B_IDLE_PRIORITY);

		ThreadEnqueuer enqueuer;
		core->RemoveCPU(cpu, enqueuer);
	}

	gCPU[cpuID].disabled = !enabled;

	if (!enabled) {
		cpu_ent* entry = &gCPU[cpuID];

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

	gCoreCount = coreCount;
	gSingleCore = coreCount == 1;
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

		gCPUEntries[i].fCPUNumber = i;
		gCPUEntries[i].fCore = core;

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

