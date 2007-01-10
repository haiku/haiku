/* Deferred Procedure Call support kernel module
 *
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin, philippe.houdoin@free.fr
 */

#include <KernelExport.h>
#include <stdlib.h>

#include <module.h>

// #include <dpc.h>

// ----
// To be moved into a new headers/os/drivers/dpc.h:
#define DPC_MODULE_NAME "generic/dpc/v1"

typedef void (*dpc_func) (void *arg);

typedef struct {
	module_info info;
	void * 		(*new_dpc_thread)(const char *name, long priority, int queue_size);
	status_t	(*delete_dpc_thread)(void *thread);
	status_t	(*queue_dpc)(void *thread, dpc_func dpc_name, void *arg);
} dpc_module_info;
// ----

// Private DPC queue structures

typedef struct {
	dpc_func 	function;
	void 		*arg;
} dpc_slot;


typedef struct {
	thread_id	thread;
	sem_id		wakeup_sem;
	spinlock	lock;
	int 		size;
	int			head;
	int 		tail;
	dpc_slot	slots[0];
	// size * slots follow
} dpc_queue;


static int32
dpc_thread(void *arg)
{
	dpc_queue *queue = arg;
	
	if (!queue)
		return -1;

	// Let's wait forever/until semaphore death for new DPC slot to show up
	while (acquire_sem(queue->wakeup_sem) == B_OK) {
		cpu_status former;
		dpc_slot call;
	
		// grab the next dpc slot
		former = disable_interrupts();
		acquire_spinlock(&queue->lock);
		
		call = queue->slots[queue->head];
		queue->head = (queue->head++) % queue->size;

		release_spinlock(&queue->lock);
		restore_interrupts(former);
		
		if (call.function)
			call.function(call.arg);
	}

	// Let's die quietly, ignored by all... sigh.
	return 0;
}

// ---- Public API

static void *
new_dpc_thread(const char *name, long priority, int queue_size)
{
	char str[64];
	dpc_queue *queue;
	
	queue = malloc(sizeof(dpc_queue) + queue_size * sizeof(dpc_slot));
	if (!queue)
		return NULL;
	
	queue->head = queue->tail = 0;
	queue->size = queue_size;
	queue->lock = 0;	// Init the spinlock
	
	strncpy(str, name, sizeof(str));
	strncat(str, "_wakeup_sem", sizeof(str));

	queue->wakeup_sem = create_sem(-1, str);
	if (queue->wakeup_sem < B_OK) {
		free(queue);
		return NULL;
	}
	set_sem_owner(queue->wakeup_sem, B_SYSTEM_TEAM);
	
	// Fire a kernel thread to actually handle (aka call them!)
	// the queued/deferred procedure calls
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

	// Wakeup the thread by murdering its favorite semaphore 
	delete_sem(queue->wakeup_sem);
	wait_for_thread(queue->thread, &exit_value);

	free(queue);
	
	return B_OK;
}


static status_t
queue_dpc(void *thread, dpc_func function, void *arg)
{
	dpc_queue *queue = thread;
	cpu_status former;
	
	if (!queue)
		return B_BAD_VALUE;

	// Try to be safe being called from interrupt handlers:
	former = disable_interrupts();
	acquire_spinlock(&queue->lock);
	
	queue->slots[queue->tail].function = function;
	queue->slots[queue->tail].arg      = arg;
	queue->tail = (queue->tail++) % queue->size;
	
	release_spinlock(&queue->lock);
	restore_interrupts(former);

	// Wake up the corresponding dpc thead
	// Notice that interrupt handlers should returns B_INVOKE_SCHEDULER to
	// shorten DPC latency as much as possible...
	return release_sem_etc(queue->wakeup_sem, 1, B_DO_NOT_RESCHEDULE);
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

