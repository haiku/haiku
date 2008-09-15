/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*! The thread scheduler */


#include <OS.h>

#include <arch/debug.h>
#include <cpu.h>
#include <debug.h>
#include <elf.h>
#include <int.h>
#include <kernel.h>
#include <kscheduler.h>
#include <scheduler_defs.h>
#include <scheduling_analysis.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <tracing.h>
#include <user_debugger.h>
#include <util/AutoLock.h>
#include <util/khash.h>


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#if SCHEDULER_TRACING
namespace SchedulerTracing {

class SchedulerTraceEntry : public AbstractTraceEntry {
public:
	SchedulerTraceEntry(struct thread* thread)
		:
		fID(thread->id)
	{
	}

	thread_id ThreadID() const	{ return fID; }

	virtual const char* Name() const = 0;

protected:
	thread_id			fID;
};


class EnqueueThread : public SchedulerTraceEntry {
public:
	EnqueueThread(struct thread* thread, struct thread* previous,
			struct thread* next)
		:
		SchedulerTraceEntry(thread),
		fPreviousID(-1),
		fNextID(-1),
		fPriority(thread->priority)
	{
		if (previous != NULL)
			fPreviousID = previous->id;
		if (next != NULL)
			fNextID = next->id;
		fName = alloc_tracing_buffer_strcpy(thread->name, B_OS_NAME_LENGTH,
			false);
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("scheduler enqueue %ld \"%s\", priority %d (previous %ld, "
			"next %ld)", fID, fName, fPriority, fPreviousID, fNextID);
	}

	virtual const char* Name() const
	{
		return fName;
	}

private:
	thread_id			fPreviousID;
	thread_id			fNextID;
	char*				fName;
	uint8				fPriority;
};


class RemoveThread : public SchedulerTraceEntry {
public:
	RemoveThread(struct thread* thread)
		:
		SchedulerTraceEntry(thread),
		fPriority(thread->priority)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("scheduler remove %ld, priority %d", fID, fPriority);
	}

	virtual const char* Name() const
	{
		return NULL;
	}

private:
	uint8				fPriority;
};


class ScheduleThread : public SchedulerTraceEntry {
public:
	ScheduleThread(struct thread* thread, struct thread* previous)
		:
		SchedulerTraceEntry(thread),
		fPreviousID(previous->id),
		fCPU(previous->cpu->cpu_num),
		fPriority(thread->priority),
		fPreviousState(previous->state),
		fPreviousWaitObjectType(previous->wait.type)
	{
		fName = alloc_tracing_buffer_strcpy(thread->name, B_OS_NAME_LENGTH,
			false);

#if SCHEDULER_TRACING >= 2
		if (fPreviousState == B_THREAD_READY)
			fPreviousPC = arch_debug_get_interrupt_pc();
		else
#endif
			fPreviousWaitObject = previous->wait.object;

		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("schedule %ld \"%s\", priority %d, CPU %ld, "
			"previous thread: %ld (", fID, fName, fPriority, fCPU, fPreviousID);
		if (fPreviousState == B_THREAD_WAITING) {
			switch (fPreviousWaitObjectType) {
				case THREAD_BLOCK_TYPE_SEMAPHORE:
					out.Print("sem %ld", (sem_id)(addr_t)fPreviousWaitObject);
					break;
				case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
					out.Print("cvar %p", fPreviousWaitObject);
					break;
				case THREAD_BLOCK_TYPE_SNOOZE:
					out.Print("snooze()");
					break;
				case THREAD_BLOCK_TYPE_SIGNAL:
					out.Print("signal");
					break;
				case THREAD_BLOCK_TYPE_MUTEX:
					out.Print("mutex %p", fPreviousWaitObject);
					break;
				case THREAD_BLOCK_TYPE_RW_LOCK:
					out.Print("rwlock %p", fPreviousWaitObject);
					break;
				case THREAD_BLOCK_TYPE_OTHER:
					out.Print("other (%p)", fPreviousWaitObject);
						// We could print the string, but it might come from a
						// kernel module that has already been unloaded.
					break;
				default:
					out.Print("unknown (%p)", fPreviousWaitObject);
					break;
			}
#if SCHEDULER_TRACING >= 2
		} else if (fPreviousState == B_THREAD_READY) {
			out.Print("ready at %p", fPreviousPC);
#endif
		} else
			out.Print("%s", thread_state_to_text(NULL, fPreviousState));

		out.Print(")");
	}

	virtual const char* Name() const
	{
		return fName;
	}

	thread_id PreviousThreadID() const		{ return fPreviousID; }
	uint8 PreviousState() const				{ return fPreviousState; }
	uint16 PreviousWaitObjectType() const	{ return fPreviousWaitObjectType; }
	const void* PreviousWaitObject() const	{ return fPreviousWaitObject; }

private:
	thread_id			fPreviousID;
	int32				fCPU;
	char*				fName;
	uint8				fPriority;
	uint8				fPreviousState;
	uint16				fPreviousWaitObjectType;
	union {
		const void*		fPreviousWaitObject;
		void*			fPreviousPC;
	};
};

}	// namespace SchedulerTracing

#	define T(x) new(std::nothrow) SchedulerTracing::x;
#else
#	define T(x) ;
#endif


// The run queue. Holds the threads ready to run ordered by priority.
static struct thread *sRunQueue = NULL;


static int
_rand(void)
{
	static int next = 0;

	if (next == 0)
		next = system_time();

	next = next * 1103515245 + 12345;
	return (next >> 16) & 0x7FFF;
}


static int
dump_run_queue(int argc, char **argv)
{
	struct thread *thread;

	thread = sRunQueue;
	if (!thread)
		kprintf("Run queue is empty!\n");
	else {
		kprintf("thread    id      priority name\n");
		while (thread) {
			kprintf("%p  %-7ld %-8ld %s\n", thread, thread->id,
				thread->priority, thread->name);
			thread = thread->queue_next;
		}
	}

	return 0;
}


#if SCHEDULER_TRACING

namespace SchedulerTracing {

enum ScheduleState {
	RUNNING,
	STILL_RUNNING,
	PREEMPTED,
	READY,
	WAITING,
	UNKNOWN
};

}


static int
cmd_scheduler(int argc, char** argv)
{
	using namespace SchedulerTracing;

	int64 threadID;
	if (argc != 2
		|| !evaluate_debug_expression(argv[1], (uint64*)&threadID, true)) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (threadID <= 0) {
		kprintf("Invalid thread ID: %lld\n", threadID);
		return 0;
	}

	ScheduleState state = UNKNOWN;
	bigtime_t lastTime = 0;

	int64 runs = 0;
	bigtime_t totalRunTime = 0;
	bigtime_t minRunTime = -1;
	bigtime_t maxRunTime = -1;

	int64 latencies = 0;
	bigtime_t totalLatency = 0;
	bigtime_t minLatency = -1;
	bigtime_t maxLatency = -1;
	int32 maxLatencyEntry = -1;

	int64 reruns = 0;
	bigtime_t totalRerunTime = 0;
	bigtime_t minRerunTime = -1;
	bigtime_t maxRerunTime = -1;
	int32 maxRerunEntry = -1;

	int64 preemptions = 0;

	TraceEntryIterator iterator;
	while (TraceEntry* _entry = iterator.Next()) {
		if (dynamic_cast<SchedulerTraceEntry*>(_entry) == NULL)
			continue;

		if (ScheduleThread* entry = dynamic_cast<ScheduleThread*>(_entry)) {
			if (entry->ThreadID() == threadID) {
				// thread scheduled
				bigtime_t diffTime = entry->Time() - lastTime;

				if (state == READY) {
					// thread scheduled after having been woken up
					latencies++;
					totalLatency += diffTime;
					if (minLatency < 0 || diffTime < minLatency)
						minLatency = diffTime;
					if (diffTime > maxLatency) {
						maxLatency = diffTime;
						maxLatencyEntry = iterator.Index();
					}
				} else if (state == PREEMPTED) {
					// thread scheduled after having been preempted before
					reruns++;
					totalRerunTime += diffTime;
					if (minRerunTime < 0 || diffTime < minRerunTime)
						minRerunTime = diffTime;
					if (diffTime > maxRerunTime) {
						maxRerunTime = diffTime;
						maxRerunEntry = iterator.Index();
					}
				}

				if (state == STILL_RUNNING) {
					// Thread was running and continues to run.
					state = RUNNING;
				}

				if (state != RUNNING) {
					lastTime = entry->Time();
					state = RUNNING;
				}
			} else if (entry->PreviousThreadID() == threadID) {
				// thread unscheduled
				bigtime_t diffTime = entry->Time() - lastTime;

				if (state == STILL_RUNNING) {
					// thread preempted
					state = PREEMPTED;

					runs++;
					preemptions++;
					totalRunTime += diffTime;
					if (minRunTime < 0 || diffTime < minRunTime)
						minRunTime = diffTime;
					if (diffTime > maxRunTime)
						maxRunTime = diffTime;
				} else if (state == RUNNING) {
					// thread starts waiting (it hadn't been added to the run
					// queue before being unscheduled)
					bigtime_t diffTime = entry->Time() - lastTime;
					runs++;
					totalRunTime += diffTime;
					if (minRunTime < 0 || diffTime < minRunTime)
						minRunTime = diffTime;
					if (diffTime > maxRunTime)
						maxRunTime = diffTime;

					state = WAITING;
				}
			}
		} else if (EnqueueThread* entry
				= dynamic_cast<EnqueueThread*>(_entry)) {
			if (entry->ThreadID() != threadID)
				continue;

			// thread enqueued in run queue

			if (state == RUNNING || state == STILL_RUNNING) {
				// Thread was running and is reentered into the run queue. This
				// is done by the scheduler, if the thread remains ready.
				state = STILL_RUNNING;
			} else {
				// Thread was waiting and is ready now.
				lastTime = entry->Time();
				state = READY;
			}
		} else if (RemoveThread* entry = dynamic_cast<RemoveThread*>(_entry)) {
			if (entry->ThreadID() != threadID)
				continue;

			// thread removed from run queue

			// This really only happens when the thread priority is changed
			// while the thread is ready.

			if (state == RUNNING) {
				// This should never happen.
				bigtime_t diffTime = entry->Time() - lastTime;
				runs++;
				totalRunTime += diffTime;
				if (minRunTime < 0 || diffTime < minRunTime)
					minRunTime = diffTime;
				if (diffTime > maxRunTime)
					maxRunTime = diffTime;
			}

			state = WAITING;
		}
	}

	// print results
	if (runs == 0) {
		kprintf("thread %lld never ran.\n", threadID);
		return 0;
	}

	kprintf("scheduling statistics for thread %lld:\n", threadID);
	kprintf("runs:\n");
	kprintf("  total #: %lld\n", runs);
	kprintf("  total:   %lld us\n", totalRunTime);
	kprintf("  average: %#.2f us\n", (double)totalRunTime / runs);
	kprintf("  min:     %lld us\n", minRunTime);
	kprintf("  max:     %lld us\n", maxRunTime);

	if (latencies > 0) {
		kprintf("scheduling latency after wake up:\n");
		kprintf("  total #: %lld\n", latencies);
		kprintf("  total:   %lld us\n", totalLatency);
		kprintf("  average: %#.2f us\n", (double)totalLatency / latencies);
		kprintf("  min:     %lld us\n", minLatency);
		kprintf("  max:     %lld us\n", maxLatency);
		kprintf("  max:     %lld us (at tracing entry %ld)\n", maxLatency,
			maxLatencyEntry);
	} else
		kprintf("thread was never run after having been woken up\n");

	if (reruns > 0) {
		kprintf("scheduling latency after preemption:\n");
		kprintf("  total #: %lld\n", reruns);
		kprintf("  total:   %lld us\n", totalRerunTime);
		kprintf("  average: %#.2f us\n", (double)totalRerunTime / reruns);
		kprintf("  min:     %lld us\n", minRerunTime);
		kprintf("  max:     %lld us (at tracing entry %ld)\n", maxRerunTime,
			maxRerunEntry);
	} else
		kprintf("thread was never rerun after preemption\n");

	if (preemptions > 0)
		kprintf("thread was preempted %lld times\n", preemptions);
	else
		kprintf("thread was never preempted\n");

	return 0;
}

#endif	// SCHEDULER_TRACING


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
void
scheduler_enqueue_in_run_queue(struct thread *thread)
{
	if (thread->state == B_THREAD_RUNNING) {
		// The thread is currently running (on another CPU) and we cannot
		// insert it into the run queue. Set the next state to ready so the
		// thread is inserted into the run queue on the next reschedule.
		thread->next_state = B_THREAD_READY;
		return;
	}

	thread->state = thread->next_state = B_THREAD_READY;

	struct thread *curr, *prev;
	for (curr = sRunQueue, prev = NULL; curr
			&& curr->priority >= thread->next_priority;
			curr = curr->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = sRunQueue;
	}

	T(EnqueueThread(thread, prev, curr));

	thread->queue_next = curr;
	if (prev)
		prev->queue_next = thread;
	else
		sRunQueue = thread;

	thread->next_priority = thread->priority;
}


/*!	Removes a thread from the run queue.
	Note: thread lock must be held when entering this function
*/
void
scheduler_remove_from_run_queue(struct thread *thread)
{
	struct thread *item, *prev;

	T(RemoveThread(thread));

	// find thread in run queue
	for (item = sRunQueue, prev = NULL; item && item != thread;
			item = item->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = sRunQueue;
	}

	ASSERT(item == thread);

	if (prev)
		prev->queue_next = item->queue_next;
	else
		sRunQueue = item->queue_next;
}


static void
context_switch(struct thread *fromThread, struct thread *toThread)
{
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_unscheduled(fromThread);

	toThread->cpu = fromThread->cpu;
	fromThread->cpu = NULL;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);

	// Looks weird, but is correct. fromThread had been unscheduled earlier,
	// but is back now. The notification for a thread scheduled the first time
	// happens in thread.cpp:thread_kthread_entry().
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(fromThread);
}


static int32
reschedule_event(timer *unused)
{
	// this function is called as a result of the timer event set by the scheduler
	// returning this causes a reschedule on the timer event
	thread_get_current_thread()->cpu->preempted = 1;
	return B_INVOKE_SCHEDULER;
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
void
scheduler_reschedule(void)
{
	struct thread *oldThread = thread_get_current_thread();
	struct thread *nextThread, *prevThread;

	TRACE(("reschedule(): cpu %d, cur_thread = %ld\n", smp_get_current_cpu(), thread_get_current_thread()->id));

	oldThread->state = oldThread->next_state;
	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			TRACE(("enqueueing thread %ld into run q. pri = %ld\n", oldThread->id, oldThread->priority));
			scheduler_enqueue_in_run_queue(oldThread);
			break;
		case B_THREAD_SUSPENDED:
			TRACE(("reschedule(): suspending thread %ld\n", oldThread->id));
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			break;
		default:
			TRACE(("not enqueueing thread %ld into run q. next_state = %ld\n", oldThread->id, oldThread->next_state));
			break;
	}

	nextThread = sRunQueue;
	prevThread = NULL;

	if (oldThread->cpu->disabled) {
		// CPU is disabled - just select an idle thread
		while (nextThread && nextThread->priority > B_IDLE_PRIORITY) {
			prevThread = nextThread;
			nextThread = nextThread->queue_next;
		}
	} else {
		while (nextThread) {
			// select next thread from the run queue
			while (nextThread && nextThread->priority > B_IDLE_PRIORITY) {
#if 0
				if (oldThread == nextThread && nextThread->was_yielded) {
					// ignore threads that called thread_yield() once
					nextThread->was_yielded = false;
					prevThread = nextThread;
					nextThread = nextThread->queue_next;
				}
#endif

				// always extract real time threads
				if (nextThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
					break;

				// never skip last non-idle normal thread
				if (nextThread->queue_next && nextThread->queue_next->priority == B_IDLE_PRIORITY)
					break;

				// skip normal threads sometimes (roughly 20%)
				if (_rand() > 0x1a00)
					break;

				// skip until next lower priority
				int32 priority = nextThread->priority;
				do {
					prevThread = nextThread;
					nextThread = nextThread->queue_next;
				} while (nextThread->queue_next != NULL
					&& priority == nextThread->queue_next->priority
					&& nextThread->queue_next->priority > B_IDLE_PRIORITY);
			}

			if (nextThread->cpu
				&& nextThread->cpu->cpu_num != oldThread->cpu->cpu_num) {
				panic("thread in run queue that's still running on another CPU!\n");
				// ToDo: remove this check completely when we're sure that this
				// cannot happen anymore.
				prevThread = nextThread;
				nextThread = nextThread->queue_next;
				continue;
			}

			break;
		}
	}

	if (!nextThread)
		panic("reschedule(): run queue is empty!\n");

	// extract selected thread from the run queue
	if (prevThread)
		prevThread->queue_next = nextThread->queue_next;
	else
		sRunQueue = nextThread->queue_next;

	T(ScheduleThread(nextThread, oldThread));

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;
	oldThread->was_yielded = false;

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	bigtime_t now = system_time();
	oldThread->kernel_time += now - oldThread->last_time;
	nextThread->last_time = now;

	// track CPU activity
	if (!thread_is_idle_thread(oldThread)) {
		oldThread->cpu->active_time +=
			(oldThread->kernel_time - oldThread->cpu->last_kernel_time)
			+ (oldThread->user_time - oldThread->cpu->last_user_time);
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		bigtime_t quantum = 3000;	// ToDo: calculate quantum!
		timer *quantumTimer = &oldThread->cpu->quantum_timer;

		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = 0;
		add_timer(quantumTimer, &reschedule_event, quantum,
			B_ONE_SHOT_RELATIVE_TIMER | B_TIMER_ACQUIRE_THREAD_LOCK);

		if (nextThread != oldThread)
			context_switch(oldThread, nextThread);
	}
}


void
scheduler_init(void)
{
	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);

#if SCHEDULER_TRACING
	add_debugger_command_etc("scheduler", &cmd_scheduler,
		"Analyze scheduler tracing information",
		"<thread>\n"
		"Analyzes scheduler tracing information for a given thread.\n"
		"  <thread>  - ID of the thread.\n", 0);
#endif
}


/*!	This starts the scheduler. Must be run under the context of
	the initial idle thread.
*/
void
scheduler_start(void)
{
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	scheduler_reschedule();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
}


// #pragma mark -


#if SCHEDULER_TRACING

namespace SchedulingAnalysis {

using namespace SchedulerTracing;

#if SCHEDULING_ANALYSIS_TRACING
using namespace SchedulingAnalysisTracing;
#endif

struct ThreadWaitObject;

struct HashObjectKey {
	virtual ~HashObjectKey()
	{
	}

	virtual uint32 HashKey() const = 0;
};


struct HashObject {
	HashObject*	next;

	virtual ~HashObject()
	{
	}

	virtual uint32 HashKey() const = 0;
	virtual bool Equals(const HashObjectKey* key) const = 0;
};


struct ThreadKey : HashObjectKey {
	thread_id	id;

	ThreadKey(thread_id id)
		:
		id(id)
	{
	}

	virtual uint32 HashKey() const
	{
		return id;
	}
};


struct Thread : HashObject, scheduling_analysis_thread {
	ScheduleState state;
	bigtime_t lastTime;

	ThreadWaitObject* waitObject;

	Thread(thread_id id)
		:
		state(UNKNOWN),
		lastTime(0),

		waitObject(NULL)
	{
		this->id = id;
		name[0] = '\0';

		runs = 0;
		total_run_time = 0;
		min_run_time = 1;
		max_run_time = -1;

		latencies = 0;
		total_latency = 0;
		min_latency = -1;
		max_latency = -1;

		reruns = 0;
		total_rerun_time = 0;
		min_rerun_time = -1;
		max_rerun_time = -1;

		unspecified_wait_time = 0;

		preemptions = 0;

		wait_objects = NULL;
	}

	virtual uint32 HashKey() const
	{
		return id;
	}

	virtual bool Equals(const HashObjectKey* _key) const
	{
		const ThreadKey* key = dynamic_cast<const ThreadKey*>(_key);
		if (key == NULL)
			return false;
		return key->id == id;
	}
};


struct WaitObjectKey : HashObjectKey {
	uint32	type;
	void*	object;

	WaitObjectKey(uint32 type, void* object)
		:
		type(type),
		object(object)
	{
	}

	virtual uint32 HashKey() const
	{
		return type ^ (uint32)(addr_t)object;
	}
};


struct WaitObject : HashObject, scheduling_analysis_wait_object {
	WaitObject(uint32 type, void* object)
	{
		this->type = type;
		this->object = object;
		name[0] = '\0';
		referenced_object = NULL;
	}

	virtual uint32 HashKey() const
	{
		return type ^ (uint32)(addr_t)object;
	}

	virtual bool Equals(const HashObjectKey* _key) const
	{
		const WaitObjectKey* key = dynamic_cast<const WaitObjectKey*>(_key);
		if (key == NULL)
			return false;
		return key->type == type && key->object == object;
	}
};


struct ThreadWaitObjectKey : HashObjectKey {
	thread_id				thread;
	uint32					type;
	void*					object;

	ThreadWaitObjectKey(thread_id thread, uint32 type, void* object)
		:
		thread(thread),
		type(type),
		object(object)
	{
	}

	virtual uint32 HashKey() const
	{
		return thread ^ type ^ (uint32)(addr_t)object;
	}
};


struct ThreadWaitObject : HashObject, scheduling_analysis_thread_wait_object {
	ThreadWaitObject(thread_id thread, WaitObject* waitObject)
	{
		this->thread = thread;
		wait_object = waitObject;
		wait_time = 0;
		waits = 0;
		next_in_list = NULL;
	}

	virtual uint32 HashKey() const
	{
		return thread ^ wait_object->type ^ (uint32)(addr_t)wait_object->object;
	}

	virtual bool Equals(const HashObjectKey* _key) const
	{
		const ThreadWaitObjectKey* key
			= dynamic_cast<const ThreadWaitObjectKey*>(_key);
		if (key == NULL)
			return false;
		return key->thread == thread && key->type == wait_object->type
			&& key->object == wait_object->object;
	}
};


class SchedulingAnalysisManager {
public:
	SchedulingAnalysisManager(void* buffer, size_t size)
		:
		fBuffer(buffer),
		fSize(size),
		fHashTable(),
		fHashTableSize(0)
	{
		fAnalysis.thread_count = 0;
		fAnalysis.threads = 0;
		fAnalysis.wait_object_count = 0;
		fAnalysis.thread_wait_object_count = 0;

		size_t maxObjectSize = max_c(max_c(sizeof(Thread), sizeof(WaitObject)),
			sizeof(ThreadWaitObject));
		fHashTableSize = size / (maxObjectSize + sizeof(HashObject*));
		fHashTable = (HashObject**)((uint8*)fBuffer + fSize) - fHashTableSize;
		fNextAllocation = (uint8*)fBuffer;
		fRemainingBytes = (addr_t)fHashTable - (addr_t)fBuffer;

		image_info info;
		if (elf_get_image_info_for_address((addr_t)&scheduler_start, &info)
				== B_OK) {
			fKernelStart = (addr_t)info.text;
			fKernelEnd = (addr_t)info.data + info.data_size;
		} else {
			fKernelStart = 0;
			fKernelEnd = 0;
		}
	}

	const scheduling_analysis* Analysis() const
	{
		return &fAnalysis;
	}

	void* Allocate(size_t size)
	{
		size = (size + 7) & ~(size_t)7;

		if (size > fRemainingBytes)
			return NULL;

		void* address = fNextAllocation;
		fNextAllocation += size;
		fRemainingBytes -= size;
		return address;
	}

	void Insert(HashObject* object)
	{
		uint32 index = object->HashKey() % fHashTableSize;
		object->next = fHashTable[index];
		fHashTable[index] = object;
	}

	void Remove(HashObject* object)
	{
		uint32 index = object->HashKey() % fHashTableSize;
		HashObject** slot = &fHashTable[index];
		while (*slot != object)
			slot = &(*slot)->next;

		*slot = object->next;
	}

	HashObject* Lookup(const HashObjectKey& key) const
	{
		uint32 index = key.HashKey() % fHashTableSize;
		HashObject* object = fHashTable[index];
		while (object != NULL && !object->Equals(&key))
			object = object->next;
		return object;
	}

	Thread* ThreadFor(thread_id id) const
	{
		return dynamic_cast<Thread*>(Lookup(ThreadKey(id)));
	}

	WaitObject* WaitObjectFor(uint32 type, void* object) const
	{
		return dynamic_cast<WaitObject*>(Lookup(WaitObjectKey(type, object)));
	}

	ThreadWaitObject* ThreadWaitObjectFor(thread_id thread, uint32 type,
		void* object) const
	{
		return dynamic_cast<ThreadWaitObject*>(
			Lookup(ThreadWaitObjectKey(thread, type, object)));
	}

	status_t AddThread(thread_id id, const char* name)
	{
		Thread* thread = ThreadFor(id);
		if (thread == NULL) {
			void* memory = Allocate(sizeof(Thread));
			if (memory == NULL)
				return B_NO_MEMORY;

			thread = new(memory) Thread(id);
			Insert(thread);
			fAnalysis.thread_count++;
		}

		if (name != NULL && thread->name[0] == '\0')
			strlcpy(thread->name, name, sizeof(thread->name));

		return B_OK;
	}

	status_t AddWaitObject(uint32 type, void* object,
		WaitObject** _waitObject = NULL)
	{
		if (WaitObjectFor(type, object) != NULL)
			return B_OK;

		void* memory = Allocate(sizeof(WaitObject));
		if (memory == NULL)
			return B_NO_MEMORY;

		WaitObject* waitObject = new(memory) WaitObject(type, object);
		Insert(waitObject);
		fAnalysis.wait_object_count++;

		// Set a dummy name for snooze() and waiting for signals, so we don't
		// try to update them later on.
		if (type == THREAD_BLOCK_TYPE_SNOOZE
			|| type == THREAD_BLOCK_TYPE_SIGNAL) {
			strcpy(waitObject->name, "?");
		}

		if (_waitObject != NULL)
			*_waitObject = waitObject;

		return B_OK;
	}

	status_t UpdateWaitObject(uint32 type, void* object, const char* name,
		void* referencedObject)
	{
		WaitObject* waitObject = WaitObjectFor(type, object);
		if (waitObject == NULL)
			return B_OK;

		if (waitObject->name[0] != '\0') {
			// This is a new object at the same address. Replace the old one.
			Remove(waitObject);
			status_t error = AddWaitObject(type, object, &waitObject);
			if (error != B_OK)
				return error;
		}

		if (name == NULL)
			name = "?";

		strlcpy(waitObject->name, name, sizeof(waitObject->name));
		waitObject->referenced_object = referencedObject;

		return B_OK;
	}

	bool UpdateWaitObjectDontAdd(uint32 type, void* object, const char* name,
		void* referencedObject)
	{
		WaitObject* waitObject = WaitObjectFor(type, object);
		if (waitObject == NULL || waitObject->name[0] != '\0')
			return false;

		if (name == NULL)
			name = "?";

		strlcpy(waitObject->name, name, sizeof(waitObject->name));
		waitObject->referenced_object = referencedObject;

		return B_OK;
	}

	status_t AddThreadWaitObject(Thread* thread, uint32 type, void* object)
	{
		WaitObject* waitObject = WaitObjectFor(type, object);
		if (waitObject == NULL) {
			// The algorithm should prevent this case.
			return B_ERROR;
		}

		ThreadWaitObject* threadWaitObject = ThreadWaitObjectFor(thread->id,
			type, object);
		if (threadWaitObject == NULL
			|| threadWaitObject->wait_object != waitObject) {
			if (threadWaitObject != NULL)
				Remove(threadWaitObject);

			void* memory = Allocate(sizeof(ThreadWaitObject));
			if (memory == NULL)
				return B_NO_MEMORY;

			threadWaitObject = new(memory) ThreadWaitObject(thread->id,
				waitObject);
			Insert(threadWaitObject);
			fAnalysis.thread_wait_object_count++;

			threadWaitObject->next_in_list = thread->wait_objects;
			thread->wait_objects = threadWaitObject;
		}

		thread->waitObject = threadWaitObject;

		return B_OK;
	}

	int32 MissingWaitObjects() const
	{
		// Iterate through the hash table and count the wait objects that don't
		// have a name yet.
		int32 count = 0;
		for (uint32 i = 0; i < fHashTableSize; i++) {
			HashObject* object = fHashTable[i];
			while (object != NULL) {
				WaitObject* waitObject = dynamic_cast<WaitObject*>(object);
				if (waitObject != NULL && waitObject->name[0] == '\0')
					count++;

				object = object->next;
			}
		}

		return count;
	}

	status_t FinishAnalysis()
	{
		// allocate the thread array
		scheduling_analysis_thread** threads
			= (scheduling_analysis_thread**)Allocate(
				sizeof(Thread*) * fAnalysis.thread_count);
		if (threads == NULL)
			return B_NO_MEMORY;

		// Iterate through the hash table and collect all threads. Also polish
		// all wait objects that haven't been update yet.
		int32 index = 0;
		for (uint32 i = 0; i < fHashTableSize; i++) {
			HashObject* object = fHashTable[i];
			while (object != NULL) {
				Thread* thread = dynamic_cast<Thread*>(object);
				if (thread != NULL) {
					threads[index++] = thread;
				} else if (WaitObject* waitObject
						= dynamic_cast<WaitObject*>(object)) {
					_PolishWaitObject(waitObject);
				}

				object = object->next;
			}
		}

		fAnalysis.threads = threads;
dprintf("scheduling analysis: free bytes: %lu/%lu\n", fRemainingBytes, fSize);
		return B_OK;
	}

private:
	void _PolishWaitObject(WaitObject* waitObject)
	{
		if (waitObject->name[0] != '\0')
			return;

		switch (waitObject->type) {
			case THREAD_BLOCK_TYPE_SEMAPHORE:
			{
				sem_info info;
				if (get_sem_info((sem_id)(addr_t)waitObject->object, &info)
						== B_OK) {
					strlcpy(waitObject->name, info.name,
						sizeof(waitObject->name));
				}
				break;
			}
			case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
			{
				// If the condition variable object is in the kernel image,
				// assume, it is still initialized.
				ConditionVariable* variable
					= (ConditionVariable*)waitObject->object;
				if (!_IsInKernelImage(variable))
					break;

				waitObject->referenced_object = (void*)variable->Object();
				strlcpy(waitObject->name, variable->ObjectType(),
					sizeof(waitObject->name));
				break;
			}

			case THREAD_BLOCK_TYPE_MUTEX:
			{
				// If the mutex object is in the kernel image, assume, it is
				// still initialized.
				mutex* lock = (mutex*)waitObject->object;
				if (!_IsInKernelImage(lock))
					break;

				strlcpy(waitObject->name, lock->name, sizeof(waitObject->name));
				break;
			}

			case THREAD_BLOCK_TYPE_RW_LOCK:
			{
				// If the mutex object is in the kernel image, assume, it is
				// still initialized.
				rw_lock* lock = (rw_lock*)waitObject->object;
				if (!_IsInKernelImage(lock))
					break;

				strlcpy(waitObject->name, lock->name, sizeof(waitObject->name));
				break;
			}

			case THREAD_BLOCK_TYPE_OTHER:
			{
				const char* name = (const char*)waitObject->object;
				if (name == NULL || _IsInKernelImage(name))
					return;

				strlcpy(waitObject->name, name, sizeof(waitObject->name));
			}

			case THREAD_BLOCK_TYPE_SNOOZE:
			case THREAD_BLOCK_TYPE_SIGNAL:
			default:
				break;
		}

		if (waitObject->name[0] != '\0')
			return;

		strcpy(waitObject->name, "?");
	}

	bool _IsInKernelImage(const void* _address)
	{
		addr_t address = (addr_t)_address;
		return address >= fKernelStart && address < fKernelEnd;
	}

private:
	scheduling_analysis	fAnalysis;
	void*				fBuffer;
	size_t				fSize;
	HashObject**		fHashTable;
	uint32				fHashTableSize;
	uint8*				fNextAllocation;
	size_t				fRemainingBytes;
	addr_t				fKernelStart;
	addr_t				fKernelEnd;
};


static status_t
analyze_scheduling(bigtime_t from, bigtime_t until,
	SchedulingAnalysisManager& manager)
{
	// analyze how much threads and locking primitives we're talking about
	TraceEntryIterator iterator;
	iterator.MoveTo(INT_MAX);
	while (TraceEntry* _entry = iterator.Previous()) {
		SchedulerTraceEntry* baseEntry
			= dynamic_cast<SchedulerTraceEntry*>(_entry);
		if (baseEntry == NULL || baseEntry->Time() >= until)
			continue;
		if (baseEntry->Time() < from)
			break;

		status_t error = manager.AddThread(baseEntry->ThreadID(),
			baseEntry->Name());
		if (error != B_OK)
			return error;

		if (ScheduleThread* entry = dynamic_cast<ScheduleThread*>(_entry)) {
			error = manager.AddThread(entry->PreviousThreadID(), NULL);
			if (error != B_OK)
				return error;

			if (entry->PreviousState() == B_THREAD_WAITING) {
				void* waitObject = (void*)entry->PreviousWaitObject();
				switch (entry->PreviousWaitObjectType()) {
					case THREAD_BLOCK_TYPE_SNOOZE:
					case THREAD_BLOCK_TYPE_SIGNAL:
						waitObject = NULL;
						break;
					case THREAD_BLOCK_TYPE_SEMAPHORE:
					case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
					case THREAD_BLOCK_TYPE_MUTEX:
					case THREAD_BLOCK_TYPE_RW_LOCK:
					case THREAD_BLOCK_TYPE_OTHER:
					default:
						break;
				}

				error = manager.AddWaitObject(entry->PreviousWaitObjectType(),
					waitObject);
				if (error != B_OK)
					return error;
			}
		}
	}

#if SCHEDULING_ANALYSIS_TRACING
	int32 startEntryIndex = iterator.Index();
#endif

	while (TraceEntry* _entry = iterator.Next()) {
#if SCHEDULING_ANALYSIS_TRACING
		// might be info on a wait object
		if (WaitObjectTraceEntry* waitObjectEntry
				= dynamic_cast<WaitObjectTraceEntry*>(_entry)) {
			status_t error = manager.UpdateWaitObject(waitObjectEntry->Type(),
				waitObjectEntry->Object(), waitObjectEntry->Name(),
				waitObjectEntry->ReferencedObject());
			if (error != B_OK)
				return error;
			continue;
		}
#endif

		SchedulerTraceEntry* baseEntry
			= dynamic_cast<SchedulerTraceEntry*>(_entry);
		if (baseEntry == NULL)
			continue;
		if (baseEntry->Time() >= until)
			break;

		if (ScheduleThread* entry = dynamic_cast<ScheduleThread*>(_entry)) {
			// scheduled thread
			Thread* thread = manager.ThreadFor(entry->ThreadID());

			bigtime_t diffTime = entry->Time() - thread->lastTime;

			if (thread->state == READY) {
				// thread scheduled after having been woken up
				thread->latencies++;
				thread->total_latency += diffTime;
				if (thread->min_latency < 0 || diffTime < thread->min_latency)
					thread->min_latency = diffTime;
				if (diffTime > thread->max_latency)
					thread->max_latency = diffTime;
			} else if (thread->state == PREEMPTED) {
				// thread scheduled after having been preempted before
				thread->reruns++;
				thread->total_rerun_time += diffTime;
				if (thread->min_rerun_time < 0
						|| diffTime < thread->min_rerun_time) {
					thread->min_rerun_time = diffTime;
				}
				if (diffTime > thread->max_rerun_time)
					thread->max_rerun_time = diffTime;
			}

			if (thread->state == STILL_RUNNING) {
				// Thread was running and continues to run.
				thread->state = RUNNING;
			}

			if (thread->state != RUNNING) {
				thread->lastTime = entry->Time();
				thread->state = RUNNING;
			}

			// unscheduled thread

			if (entry->ThreadID() == entry->PreviousThreadID())
				continue;

			thread = manager.ThreadFor(entry->PreviousThreadID());

			diffTime = entry->Time() - thread->lastTime;

			if (thread->state == STILL_RUNNING) {
				// thread preempted
				thread->runs++;
				thread->preemptions++;
				thread->total_run_time += diffTime;
				if (thread->min_run_time < 0 || diffTime < thread->min_run_time)
					thread->min_run_time = diffTime;
				if (diffTime > thread->max_run_time)
					thread->max_run_time = diffTime;

				thread->lastTime = entry->Time();
				thread->state = PREEMPTED;
			} else if (thread->state == RUNNING) {
				// thread starts waiting (it hadn't been added to the run
				// queue before being unscheduled)
				thread->runs++;
				thread->total_run_time += diffTime;
				if (thread->min_run_time < 0 || diffTime < thread->min_run_time)
					thread->min_run_time = diffTime;
				if (diffTime > thread->max_run_time)
					thread->max_run_time = diffTime;

				if (entry->PreviousState() == B_THREAD_WAITING) {
					void* waitObject = (void*)entry->PreviousWaitObject();
					switch (entry->PreviousWaitObjectType()) {
						case THREAD_BLOCK_TYPE_SNOOZE:
						case THREAD_BLOCK_TYPE_SIGNAL:
							waitObject = NULL;
							break;
						case THREAD_BLOCK_TYPE_SEMAPHORE:
						case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
						case THREAD_BLOCK_TYPE_MUTEX:
						case THREAD_BLOCK_TYPE_RW_LOCK:
						case THREAD_BLOCK_TYPE_OTHER:
						default:
							break;
					}

					status_t error = manager.AddThreadWaitObject(thread,
						entry->PreviousWaitObjectType(), waitObject);
					if (error != B_OK)
						return error;
				}

				thread->lastTime = entry->Time();
				thread->state = WAITING;
			} else if (thread->state == UNKNOWN) {
				uint32 threadState = entry->PreviousState();
				if (threadState == B_THREAD_WAITING
					|| threadState == B_THREAD_SUSPENDED) {
					thread->lastTime = entry->Time();
					thread->state = WAITING;
				} else if (threadState == B_THREAD_READY) {
					thread->lastTime = entry->Time();
					thread->state = PREEMPTED;
				}
			}
		} else if (EnqueueThread* entry
				= dynamic_cast<EnqueueThread*>(_entry)) {
			// thread enqueued in run queue

			Thread* thread = manager.ThreadFor(entry->ThreadID());

			if (thread->state == RUNNING || thread->state == STILL_RUNNING) {
				// Thread was running and is reentered into the run queue. This
				// is done by the scheduler, if the thread remains ready.
				thread->state = STILL_RUNNING;
			} else {
				// Thread was waiting and is ready now.
				bigtime_t diffTime = entry->Time() - thread->lastTime;
				if (thread->waitObject != NULL) {
					thread->waitObject->wait_time += diffTime;
					thread->waitObject->waits++;
					thread->waitObject = NULL;
				} else if (thread->state != UNKNOWN)
					thread->unspecified_wait_time += diffTime;

				thread->lastTime = entry->Time();
				thread->state = READY;
			}
		} else if (RemoveThread* entry = dynamic_cast<RemoveThread*>(_entry)) {
			// thread removed from run queue

			Thread* thread = manager.ThreadFor(entry->ThreadID());

			// This really only happens when the thread priority is changed
			// while the thread is ready.

			bigtime_t diffTime = entry->Time() - thread->lastTime;
			if (thread->state == RUNNING) {
				// This should never happen.
				thread->runs++;
				thread->total_run_time += diffTime;
				if (thread->min_run_time < 0 || diffTime < thread->min_run_time)
					thread->min_run_time = diffTime;
				if (diffTime > thread->max_run_time)
					thread->max_run_time = diffTime;
			} else if (thread->state == READY || thread->state == PREEMPTED) {
				// Not really correct, but the case is rare and we keep it
				// simple.
				thread->unspecified_wait_time += diffTime;
			}

			thread->lastTime = entry->Time();
			thread->state = WAITING;
		}
	}


#if SCHEDULING_ANALYSIS_TRACING
	int32 missingWaitObjects = manager.MissingWaitObjects();
	if (missingWaitObjects > 0) {
		iterator.MoveTo(startEntryIndex + 1);
		while (TraceEntry* _entry = iterator.Previous()) {
			if (WaitObjectTraceEntry* waitObjectEntry
					= dynamic_cast<WaitObjectTraceEntry*>(_entry)) {
				if (manager.UpdateWaitObjectDontAdd(
						waitObjectEntry->Type(), waitObjectEntry->Object(),
						waitObjectEntry->Name(),
						waitObjectEntry->ReferencedObject())) {
					if (--missingWaitObjects == 0)
						break;
				}
			}
		}
	}
#endif

	return B_OK;
}

}	// namespace SchedulingAnalysis

#endif	// SCHEDULER_TRACING


status_t
_user_analyze_scheduling(bigtime_t from, bigtime_t until, void* buffer,
	size_t size, scheduling_analysis* analysis)
{
#if SCHEDULER_TRACING
	using namespace SchedulingAnalysis;

	if ((addr_t)buffer & 0x7) {
		addr_t diff = (addr_t)buffer & 0x7;
		buffer = (void*)((addr_t)buffer + 8 - diff);
		size -= 8 - diff;
	}
	size &= ~(size_t)0x7;

	if (buffer == NULL || !IS_USER_ADDRESS(buffer) || size == 0)
		return B_BAD_VALUE;

	status_t error = lock_memory(buffer, size, B_READ_DEVICE);
	if (error != B_OK)
		return error;

	SchedulingAnalysisManager manager(buffer, size);

	InterruptsLocker locker;
	lock_tracing_buffer();

	error = analyze_scheduling(from, until, manager);

	unlock_tracing_buffer();
	locker.Unlock();

	if (error == B_OK)
		error = manager.FinishAnalysis();

	unlock_memory(buffer, size, B_READ_DEVICE);

	if (error == B_OK) {
		error = user_memcpy(analysis, manager.Analysis(),
			sizeof(scheduling_analysis));
	}

	return error;
#else
	return B_BAD_VALUE;
#endif
}
