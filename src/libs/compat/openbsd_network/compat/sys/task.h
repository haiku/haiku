/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_TASK_H_
#define _OBSD_COMPAT_SYS_TASK_H_


#include <sys/_task.h>
#include <sys/taskqueue.h>


struct taskq {
	struct taskqueue* tq;
};

#define systq (struct taskq*)(0x1)


static void
task_set(struct task *t, void (*fn)(void *), void *arg)
{
	TASK_INIT(t, 0, (task_fn_t*)fn, arg);
}


static int
task_pending(struct task *t)
{
	return t->ta_pending > 0;
}


static int
task_add(struct taskq* tasq, struct task *w)
{
	struct taskqueue* tq = (tasq == systq) ? taskqueue_fast : tasq->tq;
	if (tq == taskqueue_fast)
		w->ta_flags |= TASK_NEEDSGIANT;
	if (task_pending(w))
		return 0;
	return (taskqueue_enqueue(tq, w) == 0);
}


static struct taskq*
taskq_create(const char* name, unsigned int nthreads, int ipl, unsigned int flags)
{
	// For now, just use the system taskqueue.
	return systq;
}


static int
task_del(struct taskq* tasq, struct task *w)
{
	struct taskqueue* tq = (tasq == systq) ? taskqueue_fast : tasq->tq;
	if (!task_pending(w))
		return 0;
	return (taskqueue_cancel(tq, w, NULL) == 0);
}


#endif	/* _OBSD_COMPAT_SYS_TASK_H_ */
