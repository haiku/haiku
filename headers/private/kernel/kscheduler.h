/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H


#include <SupportDefs.h>


struct scheduling_analysis;
struct thread;


#ifdef __cplusplus
extern "C" {
#endif

void scheduler_enqueue_in_run_queue(struct thread *thread);
void scheduler_remove_from_run_queue(struct thread *thread);
void scheduler_reschedule(void);

void scheduler_init(void);
void scheduler_start(void);

status_t _user_analyze_scheduling(bigtime_t from, bigtime_t until, void* buffer,
	size_t size, struct scheduling_analysis* analysis);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_SCHEDULER_H */
