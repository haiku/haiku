/*
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
#include <kscheduler.h>
#include <thread.h>
#include <timer.h>
#include <int.h>
#include <smp.h>
#include <cpu.h>
#include <debug.h>
#include <tracing.h>
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

	thread_id PreviousThreadID() const	{ return fPreviousID; }

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

	enum ScheduleState {
		RUNNING,
		STILL_RUNNING,
		PREEMPTED,
		READY,
		WAITING,
		UNKNOWN
	};

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
			} else if (entry->PreviousThreadID() != threadID) {
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
				// is done by the scheduler, if the next thread remains ready.
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
	toThread->cpu = fromThread->cpu;
	fromThread->cpu = NULL;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);
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
