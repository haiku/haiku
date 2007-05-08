/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "device.h"

#include <compat/sys/taskqueue.h>

struct task {
	int priority;
	task_handler_t handler;
	void *argument;
};

struct taskqueue {
};

struct taskqueue *taskqueue_fast;

struct taskqueue *
taskqueue_create(const char *name, int mflags, taskqueue_enqueue_fn enqueue,
	void *context, void **unused)
{
	UNIMPLEMENTED();
	return NULL;
}


int
taskqueue_start_threads(struct taskqueue **tq, int count, int pri,
	const char *name, ...)
{
	UNIMPLEMENTED();
	return -1;
}


void
taskqueue_free(struct taskqueue *tq)
{
	UNIMPLEMENTED();
}


void
taskqueue_drain(struct taskqueue *tq, struct task *task)
{
	UNIMPLEMENTED();
}


int
taskqueue_enqueue(struct taskqueue *tq, struct task *task)
{
	UNIMPLEMENTED();
	return -1;
}


void
taskqueue_thread_enqueue(void *context)
{
	UNIMPLEMENTED();
}


int
taskqueue_enqueue_fast(struct taskqueue *queue, struct task *task)
{
	UNIMPLEMENTED();
	return -1;
}


struct taskqueue *
taskqueue_create_fast(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context)
{
	UNIMPLEMENTED();
	return NULL;
}


void
task_init(struct task *t, int prio, task_handler_t handler, void *context)
{
	t->priority = prio;
	t->handler = handler;
	t->argument = context;
}

