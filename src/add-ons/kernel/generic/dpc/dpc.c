/* Deferred Procedure Call support kernel module
 *
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin, philippe.houdoin@free.fr
 */

#include <KernelExport.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <module.h>

// #include <dpc.h>

#define DPC_MODULE_NAME "generic/dpc/v1"

typedef void (*dpc_func) (void *arg);

typedef struct {
	module_info info;
	void * 		(*new_dpc_thread)(const char *name, long priority, int max_dpc);
	status_t	(*delete_dpc_thread)(void *thread);
	status_t	(*queue_dpc)(void *thread, dpc_func dpc_name, void *arg);
} dpc_module_info;
	
// ----

typedef struct {
	dpc_func function;
	void *arg;
} dpc;

typedef struct {
	thread_id	thread;
	sem_id		wakeup_sem;
	int 		max_dpc;
	spinlock	lock;
	dpc 		list[1];
	// max_dpc - 1 follow
} dpc_queue;

static int32
dpc_thread(void *arg)
{
	// TODO:
	// loop on acquire_sem(queue->wakeup_sem), exit on bad sem
	// inner loop: disable interrupt, find first slot not empty, empty it, restore interrupts
	// and call the dpc
	return 0;
}

// ----

static void *
new_dpc_thread(const char *name, long priority, int max_dpc)
{
	char str[64];
	dpc_queue *queue;
	int i;
	
	queue = malloc(sizeof(dpc_queue) + (max_dpc - 1) * sizeof(dpc));
	if (!queue)
		return NULL;
	
	queue->max_dpc = max_dpc;
	queue->lock = 0;
	for (i = 0; i < max_dpc; i++) {
		queue->list[i].function = NULL;
		queue->list[i].arg = NULL;		
	}
	
	strncpy(str, name, sizeof(str));
	strncat(str, "_wakeup_sem", sizeof(str));
	queue->wakeup_sem = create_sem(-1, str);
	set_sem_owner(queue->wakeup_sem, B_SYSTEM_TEAM);
	
	queue->thread = spawn_kernel_thread(dpc_thread, name, priority, queue);
	if (queue->thread < 0) {
		delete_sem(queue->wakeup_sem);
		free(queue);
		return NULL;
	}
	resume_thread(queue->thread);

	return queue;
}


static status_t
delete_dpc_thread(void *thread)
{
	dpc_queue *queue = thread;
	status_t exit_value;
	
	if (!queue)
		return B_BAD_VALUE;

	delete_sem(queue->wakeup_sem);
	wait_for_thread(queue->thread, &exit_value);
	free(queue);
	
	return B_OK;
}


static status_t
queue_dpc(void *thread, dpc_func function, void *arg)
{
	dpc_queue *queue = thread;
	
	if (!queue)
		return B_BAD_VALUE;

	// TODO: acquire queue (spin)lock, find an empty slot, release lock and
	// wakup the thread (but be interrupt handler safe here: B_DO_NOT_RESCHEDULE flag!
	return B_ERROR;
}



static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}

static dpc_module_info sDPCModule = {
	{
		DPC_MODULE_NAME,
		0,
		std_ops
	},
	
	new_dpc_thread,
	delete_dpc_thread,
	queue_dpc
};


module_info *modules[] = {
	(module_info *) &sDPCModule,
	NULL
};

