/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H


#include <SupportDefs.h>


struct scheduling_analysis;
struct thread;


struct scheduler_ops {
	void (*enqueue_in_run_queue)(struct thread* thread);
	void (*reschedule)(void);
	void (*set_thread_priority)(struct thread* thread, int32 priority);
	// called when the thread structure is first created -
	// initialization of per-thread housekeeping data structures should
	// be done here
	void (*on_thread_create)(struct thread* thread);
	// called when a thread structure is initialized and made ready for
	// use - should be used to reset the housekeeping data structures
	// if needed
	void (*on_thread_init)(struct thread* thread);
	// called when a thread structure is freed - freeing up any allocated
	// mem on the scheduler's part should be done here
	void (*on_thread_destroy)(struct thread* thread);
	
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

void scheduler_init(void);

status_t _user_analyze_scheduling(bigtime_t from, bigtime_t until, void* buffer,
	size_t size, struct scheduling_analysis* analysis);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_SCHEDULER_H */
