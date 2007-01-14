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

#ifndef COMPILE_FOR_R5
	#include <stdio.h>	// For snprintf()
#endif

#include <dpc.h>

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
	int			count;
	int			head;
	int 		tail;
	dpc_slot	slots[0];
	// size * slots follow
} dpc_queue;


static int32
dpc_thread(void *arg)
{
	dpc_queue *queue = arg;
	dpc_slot dpc;
	
	// Let's wait forever/until semaphore death for new DPC slot to show up
	while (acquire_sem(queue->wakeup_sem) == B_OK) {
		cpu_status former;
	
		// grab the next dpc slot
		former = disable_interrupts();
		acquire_spinlock(&queue->lock);
		
		dpc = queue->slots[queue->head];
		queue->head = (queue->head++) % queue->size;
		queue->count--;

		release_spinlock(&queue->lock);
		restore_interrupts(former);
		
		dpc.function(dpc.arg);
	}

	// Let's finish the pending DPCs, if any.
	// Otherwise, resource could leaks...
	while (queue->count--) {
		dpc = queue->slots[queue->head];
		queue->head = (queue->head++) % queue->size;
		dpc.function(dpc.arg);	
	}

	// Now, let's die quietly, ignored by all... sigh.
	return 0;
}

// ---- Public API

static void *
new_dpc_queue(const char *name, long priority, int queue_size)
{
	char str[64];
	dpc_queue *queue;
	
	queue = malloc(sizeof(dpc_queue) + queue_size * sizeof(dpc_slot));
	if (!queue)
		return NULL;
	
	queue->head = queue->tail = 0;
	queue->size = queue_size;
	queue->count = 0;
	queue->lock = 0;	// Init the spinlock

#ifdef COMPILE_FOR_R5
	strncpy(str, name, sizeof(str) - 1);
	strncat(str, "_wakeup_sem", sizeof(str) - 1);
	str[sizeof(str) - 1] = '\0';
#else
	snprintf(str, sizeof(str), "%.*s_wakeup_sem", (int) sizeof(str) - 11, name);
#endif

	queue->wakeup_sem = create_sem(0, str);
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
delete_dpc_queue(void *handle)
{
	dpc_queue *queue = handle;
	thread_id thread;
	status_t exit_value;
	cpu_status former;

	if (!queue)
		return B_BAD_VALUE;

	// Close the queue: queue_dpc() should knows we're closing:
	former = disable_interrupts();
	acquire_spinlock(&queue->lock);
	
	thread = queue->thread;
	queue->thread = -1;
	
	release_spinlock(&queue->lock);
	restore_interrupts(former);

	// Wakeup the thread by murdering its favorite semaphore
	delete_sem(queue->wakeup_sem);
	wait_for_thread(thread, &exit_value);

	free(queue);
	
	return B_OK;
}


static status_t
queue_dpc(void *handle, dpc_func function, void *arg)
{
	dpc_queue *queue = handle;
	cpu_status former;
	status_t status = B_OK;
	
	if (!queue || !function)
		return B_BAD_VALUE;
		
	// Try to be safe being called from interrupt handlers:
	former = disable_interrupts();
	acquire_spinlock(&queue->lock);
	
	if (queue->thread < 0) {
		// Queue thread is dying...
		status = B_CANCELED;
	} else if (queue->count == queue->size)
		// This DPC queue is full, sorry
		status = B_NO_MEMORY;
	else {
		queue->slots[queue->tail].function = function;
		queue->slots[queue->tail].arg      = arg;
		queue->tail = (queue->tail++) % queue->size;
		queue->count++;	
	}

	release_spinlock(&queue->lock);
	restore_interrupts(former);

	if (status == B_OK)
		// Wake up the corresponding dpc thead
		// Notice that interrupt handlers should returns B_INVOKE_SCHEDULER to
		// shorten DPC latency as much as possible...
		status = release_sem_etc(queue->wakeup_sem, 1, B_DO_NOT_RESCHEDULE);
	
	return status;
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
		B_DPC_MODULE_NAME,
		0,
		std_ops
	},
	
	new_dpc_queue,
	delete_dpc_queue,
	queue_dpc
};


module_info *modules[] = {
	(module_info *) &sDPCModule,
	NULL
};

