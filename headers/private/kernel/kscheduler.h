/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H


#include <cpu.h>
#include <int.h>
#include <smp.h>
#include <thread_types.h>


struct scheduling_analysis;
struct SchedulerListener;


struct scheduler_ops {
	/*!	Enqueues the thread in the ready-to-run queue.
		The caller must hold the scheduler lock (with disabled interrupts).
	*/
	void (*enqueue_in_run_queue)(Thread* thread);

	/*!	Selects a thread from the ready-to-run queue and, if that's not the
		calling thread, switches the current CPU's context to run the selected
		thread.
		If it's the same thread, the thread will just continue to run.
		In either case, unless the thread is dead or is sleeping/waiting
		indefinitely, the function will eventually return.
		The caller must hold the scheduler lock (with disabled interrupts).
	*/
	void (*reschedule)(void);

	/*!	Sets the given thread's priority.
		The thread may be running or may be in the ready-to-run queue.
		The caller must hold the scheduler lock (with disabled interrupts).
	*/
	void (*set_thread_priority)(Thread* thread, int32 priority);
	bigtime_t (*estimate_max_scheduling_latency)(Thread* thread);

	/*!	Called when the Thread structure is first created.
		Per-thread housekeeping resources can be allocated.
		Interrupts must be enabled.
	*/
	status_t (*on_thread_create)(Thread* thread, bool idleThread);

	/*!	Called when a Thread structure is initialized and made ready for
		use.
		The per-thread housekeeping data structures are reset, if needed.
		The caller must hold the scheduler lock (with disabled interrupts).
	*/
	void (*on_thread_init)(Thread* thread);

	/*!	Called when a Thread structure is freed.
		Frees up any per-thread resources allocated on the scheduler's part. The
		function may be called even if on_thread_create() failed.
		Interrupts must be enabled.
	*/
	void (*on_thread_destroy)(Thread* thread);

	/*!	Called in the early boot process to start thread scheduling on the
		current CPU.
		The function is called once for each CPU.
		Interrupts must be disabled, but the caller must not hold the scheduler
		lock.
	*/
	void (*start)(void);
};

extern struct scheduler_ops* gScheduler;
extern spinlock gSchedulerLock;

#define scheduler_enqueue_in_run_queue(thread)	\
				gScheduler->enqueue_in_run_queue(thread)
#define scheduler_set_thread_priority(thread, priority)	\
				gScheduler->set_thread_priority(thread, priority)
#define scheduler_reschedule()	gScheduler->reschedule()
#define scheduler_start()		gScheduler->start()
#define scheduler_on_thread_create(thread, idleThread) \
				gScheduler->on_thread_create(thread, idleThread)
#define scheduler_on_thread_init(thread) \
				gScheduler->on_thread_init(thread)
#define scheduler_on_thread_destroy(thread) \
				gScheduler->on_thread_destroy(thread)

#ifdef __cplusplus
extern "C" {
#endif

void scheduler_add_listener(struct SchedulerListener* listener);
void scheduler_remove_listener(struct SchedulerListener* listener);

void scheduler_init(void);
void scheduler_enable_scheduling(void);

bigtime_t _user_estimate_max_scheduling_latency(thread_id thread);
status_t _user_analyze_scheduling(bigtime_t from, bigtime_t until, void* buffer,
	size_t size, struct scheduling_analysis* analysis);

#ifdef __cplusplus
}
#endif


/*!	Reschedules, if necessary.
	The caller must hold the scheduler lock (with disabled interrupts).
*/
static inline void
scheduler_reschedule_if_necessary_locked()
{
	if (gCPU[smp_get_current_cpu()].invoke_scheduler)
		scheduler_reschedule();
}


/*!	Reschedules, if necessary.
	Is a no-op, if interrupts are disabled.
*/
static inline void
scheduler_reschedule_if_necessary()
{
	if (are_interrupts_enabled()) {
		cpu_status state = disable_interrupts();
		acquire_spinlock(&gSchedulerLock);

		scheduler_reschedule_if_necessary_locked();

		release_spinlock(&gSchedulerLock);
		restore_interrupts(state);
	}
}


#endif /* KERNEL_SCHEDULER_H */
