/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <signal.h>

#include <vm_low_memory.h>
#include <vm_page.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>


//#define TRACE_LOW_MEMORY
#ifdef TRACE_LOW_MEMORY
#	define TRACE(x)	dprintf x
#else
#	define TRACE(x) ;
#endif

// TODO: the priority is currently ignored, and probably can be removed

static const bigtime_t kLowMemoryInterval = 3000000;	// 3 secs

// page limits
static const size_t kNoteLimit = 1024;
static const size_t kWarnLimit = 256;
static const size_t kCriticalLimit = 32;

static int32 sLowMemoryState = B_NO_LOW_MEMORY;


struct low_memory_handler : public DoublyLinkedListLinkImpl<low_memory_handler> {
	low_memory_func	function;
	void			*data;
	int32			priority;
};

typedef DoublyLinkedList<low_memory_handler> HandlerList;

static mutex sLowMemoryMutex;
static sem_id sLowMemoryWaitSem;
static HandlerList sLowMemoryHandlers;


static void
call_handlers(int32 level)
{
	MutexLocker locker(&sLowMemoryMutex);
	HandlerList::Iterator iterator = sLowMemoryHandlers.GetIterator();

	while (iterator.HasNext()) {
		low_memory_handler *handler = iterator.Next();

		handler->function(handler->data, level);
	}
}


static int32
compute_state(void)
{
	uint32 freePages = vm_page_num_free_pages();
	if (freePages >= kNoteLimit)
		return B_NO_LOW_MEMORY;

	// specify low memory level
	if (freePages < kCriticalLimit)
		return B_LOW_MEMORY_CRITICAL;
	else if (freePages < kWarnLimit)
		return B_LOW_MEMORY_WARNING;

	return B_LOW_MEMORY_NOTE;
}


static int32
low_memory(void *)
{
	while (true) {
		snooze(kLowMemoryInterval);

		sLowMemoryState = compute_state();

		TRACE(("vm_low_memory: state = %ld, %ld free pages\n",
			sLowMemoryState, vm_page_num_free_pages()));

		if (sLowMemoryState < B_LOW_MEMORY_NOTE)
			continue;

		call_handlers(sLowMemoryState);
	}
	return 0;
}


void
vm_low_memory(size_t requirements)
{
	// ToDo: compute level with requirements in mind

	call_handlers(B_LOW_MEMORY_NOTE);
}


int32
vm_low_memory_state(void)
{
	return sLowMemoryState;
}


status_t
vm_low_memory_init(void)
{
	thread_id thread;

	if (mutex_init(&sLowMemoryMutex, "low memory") < B_OK)
		return B_ERROR;
	
	sLowMemoryWaitSem = create_sem(0, "low memory wait");
	if (sLowMemoryWaitSem < B_OK)
		return sLowMemoryWaitSem;

	new(&sLowMemoryHandlers) HandlerList;
		// static initializers do not work in the kernel,
		// so we have to do it here, manually

	thread = spawn_kernel_thread(&low_memory, "low memory handler", B_LOW_PRIORITY, NULL);
	send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


status_t
unregister_low_memory_handler(low_memory_func function, void *data)
{
	TRACE(("unregister_low_memory_handler(function = %p, data = %p)\n",
		function, data));

	MutexLocker locker(&sLowMemoryMutex);
	HandlerList::Iterator iterator = sLowMemoryHandlers.GetIterator();

	while (iterator.HasNext()) {
		low_memory_handler *handler = iterator.Next();

		if (handler->function == function && handler->data == data) {
			sLowMemoryHandlers.Remove(handler);
			free(handler);
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
register_low_memory_handler(low_memory_func function, void *data, int32 priority)
{
	TRACE(("register_low_memory_handler(function = %p, data = %p)\n",
		function, data));

	low_memory_handler *handler = (low_memory_handler *)malloc(sizeof(low_memory_handler));
	if (handler == NULL)
		return B_NO_MEMORY;

	handler->function = function;
	handler->data = data;
	handler->priority = priority;

	MutexLocker locker(&sLowMemoryMutex);
	sLowMemoryHandlers.Add(handler);
	return B_OK;
}

