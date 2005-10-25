/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H


struct thread;


#ifdef __cplusplus
extern "C" {
#endif

void scheduler_enqueue_in_run_queue(struct thread *thread);
void scheduler_remove_from_run_queue(struct thread *thread);
void scheduler_reschedule(void);

void scheduler_init(void);
void scheduler_start(void);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_SCHEDULER_H */
