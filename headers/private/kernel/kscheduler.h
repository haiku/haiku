/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
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


#ifdef __cplusplus
extern "C" {
#endif

/*!	Enqueues the thread in the ready-to-run queue.
	The caller must hold the enqueued thread \c scheduler_lock.
*/
void scheduler_enqueue_in_run_queue(Thread* thread);

void scheduler_reschedule_ici(void);

/*!	Selects a thread from the ready-to-run queue and, if that's not the
	calling thread, switches the current CPU's context to run the selected
	thread.
	If it's the same thread, the thread will just continue to run.
	In either case, unless the thread is dead or is sleeping/waiting
	indefinitely, the function will eventually return.
	The caller must hold the current thread \c scheduler_lock.
*/
void scheduler_reschedule(int32 next_state);

/*!	Sets the given thread's priority.
	The thread may be running or may be in the ready-to-run queue.
*/
int32 scheduler_set_thread_priority(Thread* thread, int32 priority);

/*!	Called when the Thread structure is first created.
	Per-thread housekeeping resources can be allocated.
	Interrupts must be enabled.
*/
status_t scheduler_on_thread_create(Thread* thread, bool idleThread);

/*!	Called when a Thread structure is initialized and made ready for
	use.
	The per-thread housekeeping data structures are reset, if needed.
*/
void scheduler_on_thread_init(Thread* thread);

/*!	Called when a Thread structure is freed.
	Frees up any per-thread resources allocated on the scheduler's part. The
	function may be called even if on_thread_create() failed.
	Interrupts must be enabled.
*/
void scheduler_on_thread_destroy(Thread* thread);

/*!	Called in the early boot process to start thread scheduling on the
	current CPU.
	The function is called once for each CPU.
*/
void scheduler_start(void);

/*! Sets scheduler operation mode.
 */
status_t scheduler_set_operation_mode(scheduler_mode mode);

/*! Dumps scheduler specific thread information.
*/
void scheduler_dump_thread_data(Thread* thread);

void scheduler_new_thread_entry(Thread* thread);

void scheduler_set_cpu_enabled(int32 cpu, bool enabled);

void scheduler_add_listener(struct SchedulerListener* listener);
void scheduler_remove_listener(struct SchedulerListener* listener);

void scheduler_init(void);
void scheduler_enable_scheduling(void);
void scheduler_update_policy(void);

bigtime_t _user_estimate_max_scheduling_latency(thread_id thread);
status_t _user_analyze_scheduling(bigtime_t from, bigtime_t until, void* buffer,
	size_t size, struct scheduling_analysis* analysis);

status_t _user_set_scheduler_mode(int32 mode);
int32 _user_get_scheduler_mode(void);

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
		scheduler_reschedule(B_THREAD_READY);
}


/*!	Reschedules, if necessary.
	Is a no-op, if interrupts are disabled.
*/
static inline void
scheduler_reschedule_if_necessary()
{
	if (are_interrupts_enabled()) {
		cpu_status state = disable_interrupts();

		Thread* thread = get_cpu_struct()->running_thread;
		acquire_spinlock(&thread->scheduler_lock);

		scheduler_reschedule_if_necessary_locked();

		release_spinlock(&thread->scheduler_lock);

		restore_interrupts(state);
	}
}


#endif /* KERNEL_SCHEDULER_H */
