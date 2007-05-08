#ifndef _FBSD_COMPAT_SYS_TASKQUEUE_H_
#define _FBSD_COMPAT_SYS_TASKQUEUE_H_

#include <sys/kernel.h>

#define PI_NET	0

struct task;
struct taskqueue;

typedef void (*task_handler_t)(void *context, int pending);

#define TASK_INIT(taskp, prio, hand, arg) task_init(taskp, prio, hand, arg)

typedef void (*taskqueue_enqueue_fn)(void *context);

struct taskqueue *taskqueue_create(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context, void **);
int taskqueue_start_threads(struct taskqueue **tq, int count, int pri,
	const char *name, ...) __printflike(4, 5);
void taskqueue_free(struct taskqueue *tq);
void taskqueue_drain(struct taskqueue *tq, struct task *task);

int taskqueue_enqueue(struct taskqueue *tq, struct task *task);

void taskqueue_thread_enqueue(void *context);

extern struct taskqueue *taskqueue_fast;
int taskqueue_enqueue_fast(struct taskqueue *queue, struct task *task);
struct taskqueue *taskqueue_create_fast(const char *name, int mflags,
	taskqueue_enqueue_fn enqueue, void *context);

void task_init(struct task *, int prio, task_handler_t handler, void *arg);

#endif
