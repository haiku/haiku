/*
 * Copyright 2007-2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_TASKQUEUE_H_
#define _FBSD_COMPAT_SYS_TASKQUEUE_H_


#include <sys/queue.h>
#include <sys/_task.h>
#include <sys/callout.h>


struct timeout_task {
	struct taskqueue *q;
	struct task t;
	struct callout c;
	int    f;
};

#define	TASKQUEUE_NAMELEN		64

typedef void (*taskqueue_enqueue_fn)(void *context);


struct taskqueue;
struct taskqueue *taskqueue_create(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context);
int taskqueue_start_threads(struct taskqueue **tq, int count, int pri,
	const char *name, ...) __printflike(4, 5);
void taskqueue_free(struct taskqueue *tq);
void taskqueue_drain(struct taskqueue *tq, struct task *task);
void taskqueue_drain_timeout(struct taskqueue *queue,
	struct timeout_task *timeout_task);
void taskqueue_drain_all(struct taskqueue *tq);
void taskqueue_block(struct taskqueue *queue);
void taskqueue_unblock(struct taskqueue *queue);
int taskqueue_enqueue(struct taskqueue *tq, struct task *task);
int taskqueue_enqueue_timeout(struct taskqueue *queue,
	struct timeout_task *ttask, int _ticks);
int taskqueue_cancel(struct taskqueue *queue, struct task *task,
	u_int *pendp);
int taskqueue_cancel_timeout(struct taskqueue *queue,
	struct timeout_task *timeout_task, u_int *pendp);

void taskqueue_thread_enqueue(void *context);

extern struct taskqueue *taskqueue_fast;
extern struct taskqueue *taskqueue_swi;
extern struct taskqueue *taskqueue_thread;

int taskqueue_enqueue_fast(struct taskqueue *queue, struct task *task);
struct taskqueue *taskqueue_create_fast(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context);

void task_init(struct task *, int prio, task_fn_t handler, void *arg);
#define TASK_INIT(taskp, prio, hand, arg) task_init(taskp, prio, hand, arg)

void timeout_task_init(struct taskqueue *queue, struct timeout_task *timeout_task,
	int priority, task_fn_t func, void *context);
#define	TIMEOUT_TASK_INIT(queue, timeout_task, priority, func, context) \
	timeout_task_init(queue, timeout_task, priority, func, context);

#endif
