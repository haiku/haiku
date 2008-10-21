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

	void (*start)(void);
};

extern struct scheduler_ops* gScheduler;

#define scheduler_enqueue_in_run_queue(thread)	\
				gScheduler->enqueue_in_run_queue(thread)
#define scheduler_set_thread_priority(thread, priority)	\
				gScheduler->set_thread_priority(thread, priority)
#define scheduler_reschedule()	gScheduler->reschedule()
#define scheduler_start()		gScheduler->start()


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
