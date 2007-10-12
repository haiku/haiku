/*
 * Copyright 2004-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Thread definition and structures
 */
#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H

#define _THREAD_H

#include <OS.h>

#if 0
struct cpu_ent {
	bool preempted;
};
#endif
extern struct thread_queue dead_q;

struct thread {
	struct thread	*queue_next;	/* i.e. run queue, release queue, etc. */
	char			*name;
	thread_id		id;
	int32			priority;
	int32			next_priority;
	int32			state;
	int32			next_state;
	struct cpu_ent	*cpu;
	struct Thread	*object;

	bool			in_kernel;
	bool			is_idle;
	bool			was_yielded;

	bigtime_t		user_time;
	bigtime_t		kernel_time;
	bigtime_t		last_time;
};

enum additional_thread_state {
	THREAD_STATE_FREE_ON_RESCHED = 7, // free the thread structure upon reschedule
//	THREAD_STATE_BIRTH	// thread is being created
};

struct thread_queue {
	struct thread *head;
	struct thread *tail;
};


static inline bool
thread_is_idle_thread(struct thread *thread)
{
	return thread->is_idle;
}

void thread_enqueue(struct thread *t, struct thread_queue *q);
struct thread *thread_get_current_thread(void);

#define GRAB_THREAD_LOCK grab_thread_lock
#define RELEASE_THREAD_LOCK release_thread_lock
void grab_thread_lock(void);
void release_thread_lock(void);

void arch_thread_context_switch(struct thread *t_from, struct thread *t_to);
void arch_thread_set_current_thread(struct thread *t);

#endif	/* _KERNEL_THREAD_TYPES_H */
