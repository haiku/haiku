/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_TASKQUEUE_H_
#define _FBSD_COMPAT_SYS_TASKQUEUE_H_


#include <sys/queue.h>
#include <sys/_task.h>


#define PI_NET	(B_REAL_TIME_DISPLAY_PRIORITY - 1)

#define TASK_INIT(taskp, prio, hand, arg) task_init(taskp, prio, hand, arg)


typedef void (*taskqueue_enqueue_fn)(void *context);


struct taskqueue;
struct taskqueue *taskqueue_create(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context);
int taskqueue_start_threads(struct taskqueue **tq, int count, int pri,
	const char *name, ...) __printflike(4, 5);
void taskqueue_free(struct taskqueue *tq);
void taskqueue_drain(struct taskqueue *tq, struct task *task);
void taskqueue_block(struct taskqueue *queue);
void taskqueue_unblock(struct taskqueue *queue);
int taskqueue_enqueue(struct taskqueue *tq, struct task *task);

void taskqueue_thread_enqueue(void *context);

extern struct taskqueue *taskqueue_fast;
extern struct taskqueue *taskqueue_swi;

int taskqueue_enqueue_fast(struct taskqueue *queue, struct task *task);
struct taskqueue *taskqueue_create_fast(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context);

void task_init(struct task *, int prio, task_handler_t handler, void *arg);

#endif
