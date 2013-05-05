/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */


#include "device.h"

#include <stdio.h>

#include <compat/sys/taskqueue.h>
#include <compat/sys/haiku-module.h>


#define TQ_FLAGS_ACTIVE		(1 << 0)
#define TQ_FLAGS_BLOCKED	(1 << 1)
#define TQ_FLAGS_PENDING	(1 << 2)


struct taskqueue {
	char tq_name[64];
	mutex tq_mutex;
	struct list tq_list;
	taskqueue_enqueue_fn tq_enqueue;
	void *tq_arg;
	int tq_fast;
	spinlock tq_spinlock;
	sem_id tq_sem;
	thread_id *tq_threads;
	thread_id tq_thread_storage;
	int tq_threadcount;
	int tq_flags;
};

struct taskqueue *taskqueue_fast = NULL;
struct taskqueue *taskqueue_swi = NULL;


static struct taskqueue *
_taskqueue_create(const char *name, int mflags, int fast,
	taskqueue_enqueue_fn enqueueFunction, void *context)
{
	struct taskqueue *tq = malloc(sizeof(struct taskqueue));
	if (tq == NULL)
		return NULL;

	tq->tq_fast = fast;

	if (fast) {
		B_INITIALIZE_SPINLOCK(&tq->tq_spinlock);
	} else {
		mutex_init_etc(&tq->tq_mutex, name, MUTEX_FLAG_CLONE_NAME);
	}

	strlcpy(tq->tq_name, name, sizeof(tq->tq_name));
	list_init_etc(&tq->tq_list, offsetof(struct task, ta_link));
	tq->tq_enqueue = enqueueFunction;
	tq->tq_arg = context;

	tq->tq_sem = -1;
	tq->tq_threads = NULL;
	tq->tq_threadcount = 0;
	tq->tq_flags = TQ_FLAGS_ACTIVE;

	return tq;
}


static void
tq_lock(struct taskqueue *taskQueue, cpu_status *status)
{
	if (taskQueue->tq_fast) {
		*status = disable_interrupts();
		acquire_spinlock(&taskQueue->tq_spinlock);
	} else {
		mutex_lock(&taskQueue->tq_mutex);
	}
}


static void
tq_unlock(struct taskqueue *taskQueue, cpu_status status)
{
	if (taskQueue->tq_fast) {
		release_spinlock(&taskQueue->tq_spinlock);
		restore_interrupts(status);
	} else {
		mutex_unlock(&taskQueue->tq_mutex);
	}
}


struct taskqueue *
taskqueue_create(const char *name, int mflags,
	taskqueue_enqueue_fn enqueueFunction, void *context)
{
	return _taskqueue_create(name, mflags, 0, enqueueFunction, context);
}


static int32
tq_handle_thread(void *data)
{
	struct taskqueue *tq = data;
	cpu_status cpu_state;
	struct task *t;
	int pending;
	sem_id sem;

	/* just a synchronization point */
	tq_lock(tq, &cpu_state);
	sem = tq->tq_sem;
	tq_unlock(tq, cpu_state);

	while (acquire_sem(sem) == B_NO_ERROR) {
		tq_lock(tq, &cpu_state);
		t = list_remove_head_item(&tq->tq_list);
		tq_unlock(tq, cpu_state);
		if (t == NULL)
			continue;
		pending = t->ta_pending;
		t->ta_pending = 0;

		t->ta_handler(t->ta_argument, pending);
	}

	return 0;
}


static int
_taskqueue_start_threads(struct taskqueue **taskQueue, int count, int priority,
	const char *name)
{
	struct taskqueue *tq = (*taskQueue);
	int i, j;

	if (count == 0)
		return -1;

	if (tq->tq_threads != NULL)
		return -1;

	if (count == 1) {
		tq->tq_threads = &tq->tq_thread_storage;
	} else {
		tq->tq_threads = malloc(sizeof(thread_id) * count);
		if (tq->tq_threads == NULL)
			return B_NO_MEMORY;
	}

	tq->tq_sem = create_sem(0, tq->tq_name);
	if (tq->tq_sem < B_OK) {
		if (count > 1)
			free(tq->tq_threads);
		tq->tq_threads = NULL;
		return tq->tq_sem;
	}

	for (i = 0; i < count; i++) {
		tq->tq_threads[i] = spawn_kernel_thread(tq_handle_thread, tq->tq_name,
			priority, tq);
		if (tq->tq_threads[i] < B_OK) {
			status_t status = tq->tq_threads[i];
			for (j = 0; j < i; j++)
				kill_thread(tq->tq_threads[j]);
			if (count > 1)
				free(tq->tq_threads);
			tq->tq_threads = NULL;
			delete_sem(tq->tq_sem);
			return status;
		}
	}

	tq->tq_threadcount = count;

	for (i = 0; i < count; i++)
		resume_thread(tq->tq_threads[i]);

	return 0;
}


int
taskqueue_start_threads(struct taskqueue **taskQueue, int count, int priority,
	const char *format, ...)
{
	/* we assume that start_threads is called in a sane place, and thus
	 * don't need to be locked. This is mostly due to the fact that if
	 * the TQ is 'fast', locking the TQ disables interrupts... and then
	 * we can't create semaphores, threads and bananas. */

	/* cpu_status state; */
	char name[64];
	int result;
	va_list vl;

	va_start(vl, format);
	vsnprintf(name, sizeof(name), format, vl);
	va_end(vl);

	/*tq_lock(*tqp, &state);*/
	result = _taskqueue_start_threads(taskQueue, count, priority, name);
	/*tq_unlock(*tqp, state);*/

	return result;
}


void
taskqueue_free(struct taskqueue *taskQueue)
{
	/* lock and  drain list? */
	taskQueue->tq_flags &= ~TQ_FLAGS_ACTIVE;
	if (!taskQueue->tq_fast)
		mutex_destroy(&taskQueue->tq_mutex);
	if (taskQueue->tq_sem != -1) {
		int i;

		delete_sem(taskQueue->tq_sem);

		for (i = 0; i < taskQueue->tq_threadcount; i++) {
			status_t status;
			wait_for_thread(taskQueue->tq_threads[i], &status);
		}

		if (taskQueue->tq_threadcount > 1)
			free(taskQueue->tq_threads);
	}

	free(taskQueue);
}


void
taskqueue_drain(struct taskqueue *taskQueue, struct task *task)
{
	cpu_status status;

	tq_lock(taskQueue, &status);
	while (task->ta_pending != 0) {
		tq_unlock(taskQueue, status);
		snooze(0);
		tq_lock(taskQueue, &status);
	}
	tq_unlock(taskQueue, status);
}


int
taskqueue_enqueue(struct taskqueue *taskQueue, struct task *task)
{
	cpu_status status;
	tq_lock(taskQueue, &status);
	/* we don't really support priorities */
	if (task->ta_pending) {
		task->ta_pending++;
	} else {
		list_add_item(&taskQueue->tq_list, task);
		task->ta_pending = 1;
		if ((taskQueue->tq_flags & TQ_FLAGS_BLOCKED) == 0)
			taskQueue->tq_enqueue(taskQueue->tq_arg);
		else
			taskQueue->tq_flags |= TQ_FLAGS_PENDING;
	}
	tq_unlock(taskQueue, status);
	return 0;
}


void
taskqueue_thread_enqueue(void *context)
{
	struct taskqueue **tqp = context;
	release_sem_etc((*tqp)->tq_sem, 1, B_DO_NOT_RESCHEDULE);
}


int
taskqueue_enqueue_fast(struct taskqueue *taskQueue, struct task *task)
{
	return taskqueue_enqueue(taskQueue, task);
}


struct taskqueue *
taskqueue_create_fast(const char *name, int mflags,
	taskqueue_enqueue_fn enqueueFunction, void *context)
{
	return _taskqueue_create(name, mflags, 1, enqueueFunction, context);
}


void
task_init(struct task *task, int prio, task_handler_t handler, void *context)
{
	task->ta_priority = prio;
	task->ta_handler = handler;
	task->ta_argument = context;
	task->ta_pending = 0;
}


status_t
init_taskqueues()
{
	status_t status = B_NO_MEMORY;

	if (HAIKU_DRIVER_REQUIRES(FBSD_FAST_TASKQUEUE)) {
		taskqueue_fast = taskqueue_create_fast("fast taskq", 0,
			taskqueue_thread_enqueue, &taskqueue_fast);
		if (taskqueue_fast == NULL)
			return B_NO_MEMORY;

		status = taskqueue_start_threads(&taskqueue_fast, 1,
			B_REAL_TIME_PRIORITY, "fast taskq thread");
		if (status < B_OK)
			goto err_1;
	}

	if (HAIKU_DRIVER_REQUIRES(FBSD_SWI_TASKQUEUE)) {
		taskqueue_swi = taskqueue_create_fast("swi taskq", 0,
			taskqueue_thread_enqueue, &taskqueue_swi);
		if (taskqueue_swi == NULL) {
			status = B_NO_MEMORY;
			goto err_1;
		}

		status = taskqueue_start_threads(&taskqueue_swi, 1,
			B_REAL_TIME_PRIORITY, "swi taskq");
		if (status < B_OK)
			goto err_2;
	}

	return B_OK;

err_2:
	if (taskqueue_swi)
		taskqueue_free(taskqueue_swi);

err_1:
	if (taskqueue_fast)
		taskqueue_free(taskqueue_fast);

	return status;
}


void
uninit_taskqueues()
{
	if (HAIKU_DRIVER_REQUIRES(FBSD_SWI_TASKQUEUE))
		taskqueue_free(taskqueue_swi);

	if (HAIKU_DRIVER_REQUIRES(FBSD_FAST_TASKQUEUE))
		taskqueue_free(taskqueue_fast);
}


void
taskqueue_block(struct taskqueue *taskQueue)
{
	cpu_status status;

	tq_lock(taskQueue, &status);
	taskQueue->tq_flags |= TQ_FLAGS_BLOCKED;
	tq_unlock(taskQueue, status);
}


void
taskqueue_unblock(struct taskqueue *taskQueue)
{
	cpu_status status;

	tq_lock(taskQueue, &status);
	taskQueue->tq_flags &= ~TQ_FLAGS_BLOCKED;
	if (taskQueue->tq_flags & TQ_FLAGS_PENDING) {
		taskQueue->tq_flags &= ~TQ_FLAGS_PENDING;
		taskQueue->tq_enqueue(taskQueue->tq_arg);
	}
	tq_unlock(taskQueue, status);
}
