/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "device.h"

#include <stdio.h>
#include <util/list.h>

#include <compat/sys/taskqueue.h>
#include <compat/sys/haiku-module.h>

struct task {
	int ta_priority;
	task_handler_t ta_handler;
	void *ta_argument;
	int ta_pending;

	struct list_link ta_link;
};

struct taskqueue {
	char tq_name[64];
	mutex tq_mutex;
	struct list tq_list;
	taskqueue_enqueue_fn tq_enqueue;
	void *tq_arg;
	int tq_fast;
	int32 tq_spinlock;
	sem_id tq_sem;
	thread_id *tq_threads;
	int tq_threadcount;
};


struct taskqueue *taskqueue_fast = NULL;


static struct taskqueue *
_taskqueue_create(const char *name, int mflags, int fast,
	taskqueue_enqueue_fn enqueue, void *context)
{
	struct taskqueue *tq = malloc(sizeof(struct taskqueue));
	if (tq == NULL)
		return NULL;

	tq->tq_fast = fast;

	if (fast) {
		tq->tq_spinlock = 0;
	} else {
		if (mutex_init(&tq->tq_mutex, name) < B_OK) {
			free(tq);
			return NULL;
		}
	}

	strlcpy(tq->tq_name, name, sizeof(tq->tq_name));
	list_init_etc(&tq->tq_list, offsetof(struct task, ta_link));
	tq->tq_enqueue = enqueue;
	tq->tq_arg = context;

	tq->tq_sem = -1;
	tq->tq_threads = NULL;
	tq->tq_threadcount = 0;

	return tq;
}


static void
tq_lock(struct taskqueue *tq, cpu_status *status)
{
	if (tq->tq_fast) {
		*status = disable_interrupts();
		acquire_spinlock(&tq->tq_spinlock);
	} else {
		mutex_lock(&tq->tq_mutex);
	}
}


static void
tq_unlock(struct taskqueue *tq, cpu_status status)
{
	if (tq->tq_fast) {
		release_spinlock(&tq->tq_spinlock);
		restore_interrupts(status);
	} else {
		mutex_unlock(&tq->tq_mutex);
	}
}


struct taskqueue *
taskqueue_create(const char *name, int mflags, taskqueue_enqueue_fn enqueue,
	void *context, void **unused)
{
	return _taskqueue_create(name, mflags, 0, enqueue, context);
}


static int32
tq_handle_thread(void *data)
{
	struct taskqueue *tq = data;
	cpu_status cpu_state;
	struct task *t;
	int pending;

	while (1) {
		status_t status = acquire_sem(tq->tq_sem);
		if (status < B_OK)
			break;

		tq_lock(tq, &cpu_state);
		t = list_remove_head_item(&tq->tq_list);
		pending = t->ta_pending;
		t->ta_pending = 0;
		tq_unlock(tq, cpu_state);

		t->ta_handler(t->ta_argument, pending);
	}

	return 0;
}


int
taskqueue_start_threads(struct taskqueue **tqp, int count, int pri,
	const char *format, ...)
{
	struct taskqueue *tq = (*tqp);
	char name[64];
	va_list vl;
	int i, j;

	if (count == 0)
		return -1;

	if (tq->tq_threads != NULL)
		return -1;

	va_start(vl, format);
	vsnprintf(name, sizeof(name), format, vl);
	va_end(vl);

	tq->tq_threads = malloc(sizeof(thread_id) * count);
	if (tq->tq_threads == NULL)
		return B_NO_MEMORY;

	tq->tq_sem = create_sem(0, tq->tq_name);
	if (tq->tq_sem < B_OK) {
		free(tq->tq_threads);
		tq->tq_threads = NULL;
		return tq->tq_sem;
	}

	for (i = 0; i < count; i++) {
		tq->tq_threads[i] = spawn_kernel_thread(tq_handle_thread, tq->tq_name,
			B_REAL_TIME_DISPLAY_PRIORITY - 20, tq);
		if (tq->tq_threads[i] < B_OK) {
			status_t status = tq->tq_threads[i];
			for (j = 0; j < i; j++)
				kill_thread(tq->tq_threads[j]);
			free(tq->tq_threads);
			tq->tq_threads = NULL;
			delete_sem(tq->tq_sem);
			return status;
		}
	}

	for (i = 0; i < count; i++)
		resume_thread(tq->tq_threads[i]);

	return 0;
}


void
taskqueue_free(struct taskqueue *tq)
{
	/* lock and  drain list? */
	if (!tq->tq_fast)
		mutex_destroy(&tq->tq_mutex);
	if (tq->tq_sem != -1) {
		int i;

		delete_sem(tq->tq_sem);

		for (i = 0; i < tq->tq_threadcount; i++) {
			status_t status;
			wait_for_thread(tq->tq_threads[i], &status);
		}
	}

	free(tq);
}


void
taskqueue_drain(struct taskqueue *tq, struct task *task)
{
	cpu_status status;

	tq_lock(tq, &status);
	if (task->ta_pending != 0)
		panic("unimplemented, taskqueue drain");
	tq_unlock(tq, status);
}


int
taskqueue_enqueue(struct taskqueue *tq, struct task *task)
{
	cpu_status status;
	tq_lock(tq, &status);
	/* we don't really support priorities */
	if (task->ta_pending) {
		task->ta_pending++;
	} else {
		list_add_item(&tq->tq_list, task);
		task->ta_pending = 1;
		tq->tq_enqueue(tq->tq_arg);
	}
	tq_unlock(tq, status);
	return 0;
}


void
taskqueue_thread_enqueue(void *context)
{
	struct taskqueue **tqp = context;
	release_sem_etc((*tqp)->tq_sem, 1, B_DO_NOT_RESCHEDULE);
}


int
taskqueue_enqueue_fast(struct taskqueue *tq, struct task *task)
{
	return taskqueue_enqueue(tq, task);
}


struct taskqueue *
taskqueue_create_fast(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context)
{
	return _taskqueue_create(name, mflags, 1, enqueue, context);
}


void
task_init(struct task *t, int prio, task_handler_t handler, void *context)
{
	t->ta_priority = prio;
	t->ta_handler = handler;
	t->ta_argument = context;
	t->ta_pending = 0;
}


status_t
init_taskqueues()
{
	if (HAIKU_DRIVER_REQUIRES(FBSD_FAST_TASKQUEUE)) {
		taskqueue_fast = taskqueue_create_fast("fast taskq", 0,
			taskqueue_thread_enqueue, NULL);
		if (taskqueue_fast == NULL)
			return B_NO_MEMORY;

		if (taskqueue_start_threads(&taskqueue_fast, 1, 0, "fast taskq") < 0) {
			taskqueue_free(taskqueue_fast);
			return B_ERROR;
		}
	}

	return B_OK;
}


void
uninit_taskqueues()
{
	if (HAIKU_DRIVER_REQUIRES(FBSD_FAST_TASKQUEUE))
		taskqueue_free(taskqueue_fast);
}
