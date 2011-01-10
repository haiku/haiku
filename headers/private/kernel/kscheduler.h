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
	void (*enqueue_in_run_queue)(Thread* thread);
	void (*reschedule)(void);
	void (*set_thread_priority)(Thread* thread, int32 priority);
	bigtime_t (*estimate_max_scheduling_latency)(Thread* thread);

	void (*on_thread_create)(Thread* thread);
		// called when the thread structure is first created -
		// initialization of per-thread housekeeping data structures should
		// be done here
	void (*on_thread_init)(Thread* thread);
		// called when a thread structure is initialized and made ready for
		// use - should be used to reset the housekeeping data structures
		// if needed
	void (*on_thread_destroy)(Thread* thread);
		// called when a thread structure is freed - freeing up any allocated
		// mem on the scheduler's part should be done here

	void (*start)(void);
};

extern struct scheduler_ops* gScheduler;

#define scheduler_enqueue_in_run_queue(thread)	\
				gScheduler->enqueue_in_run_queue(thread)
#define scheduler_set_thread_priority(thread, priority)	\
				gScheduler->set_thread_priority(thread, priority)
#define scheduler_reschedule()	gScheduler->reschedule()
#define scheduler_start()		gScheduler->start()
#define scheduler_on_thread_create(thread) \
				gScheduler->on_thread_create(thread)
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
	The thread spinlock must be held.
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
		GRAB_THREAD_LOCK();
		scheduler_reschedule_if_necessary_locked();
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}
}


#endif /* KERNEL_SCHEDULER_H */
