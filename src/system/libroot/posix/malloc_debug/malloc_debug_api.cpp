/*
 * Copyright 2015, Michael Lotz <mmlr@mlotz.ch>.
 * Distributed under the terms of the MIT License.
 */


#include "malloc_debug_api.h"

#include <malloc.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>


static heap_implementation* sCurrentHeap = NULL;


// #pragma mark - Heap Debug API


extern "C" status_t
heap_debug_start_wall_checking(int msInterval)
{
	if (sCurrentHeap->start_wall_checking != NULL)
		return sCurrentHeap->start_wall_checking(msInterval);

	return B_NOT_SUPPORTED;
}


extern "C" status_t
heap_debug_stop_wall_checking()
{
	if (sCurrentHeap->stop_wall_checking != NULL)
		return sCurrentHeap->stop_wall_checking();

	return B_NOT_SUPPORTED;
}


extern "C" void
heap_debug_set_paranoid_validation(bool enabled)
{
	if (sCurrentHeap->set_paranoid_validation != NULL)
		sCurrentHeap->set_paranoid_validation(enabled);
}


extern "C" void
heap_debug_set_memory_reuse(bool enabled)
{
	if (sCurrentHeap->set_memory_reuse != NULL)
		sCurrentHeap->set_memory_reuse(enabled);
}


extern "C" void
heap_debug_set_debugger_calls(bool enabled)
{
	if (sCurrentHeap->set_debugger_calls != NULL)
		sCurrentHeap->set_debugger_calls(enabled);
}


extern "C" void
heap_debug_set_default_alignment(size_t defaultAlignment)
{
	if (sCurrentHeap->set_default_alignment != NULL)
		sCurrentHeap->set_default_alignment(defaultAlignment);
}


extern "C" void
heap_debug_validate_heaps()
{
	if (sCurrentHeap->validate_heaps != NULL)
		sCurrentHeap->validate_heaps();
}


extern "C" void
heap_debug_validate_walls()
{
	if (sCurrentHeap->validate_walls != NULL)
		sCurrentHeap->validate_walls();
}


extern "C" void
heap_debug_dump_allocations(bool statsOnly, thread_id thread)
{
	if (sCurrentHeap->dump_allocations != NULL)
		sCurrentHeap->dump_allocations(statsOnly, thread);
}


extern "C" void
heap_debug_dump_heaps(bool dumpAreas, bool dumpBins)
{
	if (sCurrentHeap->dump_heaps != NULL)
		sCurrentHeap->dump_heaps(dumpAreas, dumpBins);
}


extern "C" void *
heap_debug_malloc_with_guard_page(size_t size)
{
	if (sCurrentHeap->malloc_with_guard_page != NULL)
		return sCurrentHeap->malloc_with_guard_page(size);

	return NULL;
}


extern "C" status_t
heap_debug_get_allocation_info(void *address, size_t *size,
	thread_id *thread)
{
	if (sCurrentHeap->get_allocation_info != NULL)
		return sCurrentHeap->get_allocation_info(address, size, thread);

	return B_NOT_SUPPORTED;
}


extern "C" status_t
heap_debug_set_dump_allocations_on_exit(bool enabled)
{
	if (sCurrentHeap->set_dump_allocations_on_exit != NULL)
		return sCurrentHeap->set_dump_allocations_on_exit(enabled);

	return B_NOT_SUPPORTED;
}


extern "C" status_t
heap_debug_set_stack_trace_depth(size_t stackTraceDepth)
{
	if (sCurrentHeap->set_stack_trace_depth != NULL)
		return sCurrentHeap->set_stack_trace_depth(stackTraceDepth);

	return B_NOT_SUPPORTED;
}


// #pragma mark - Init


extern "C" status_t
__init_heap(void)
{
	const char *mode = getenv("MALLOC_DEBUG");
	if (mode == NULL || strchr(mode, 'g') == NULL)
		sCurrentHeap = &__mallocDebugHeap;
	else
		sCurrentHeap = &__mallocGuardedHeap;

	status_t result = sCurrentHeap->init();
	if (result != B_OK)
		return result;

	if (mode != NULL) {
		if (strchr(mode, 'p') != NULL)
			heap_debug_set_paranoid_validation(true);
		if (strchr(mode, 'r') != NULL)
			heap_debug_set_memory_reuse(false);
		if (strchr(mode, 'e') != NULL)
			heap_debug_set_dump_allocations_on_exit(true);

		size_t defaultAlignment = 0;
		const char *argument = strchr(mode, 'a');
		if (argument != NULL
			&& sscanf(argument, "a%" B_SCNuSIZE, &defaultAlignment) == 1) {
			heap_debug_set_default_alignment(defaultAlignment);
		}

		size_t stackTraceDepth = 0;
		argument = strchr(mode, 's');
		if (argument != NULL
			&& sscanf(argument, "s%" B_SCNuSIZE, &stackTraceDepth) == 1) {
			heap_debug_set_stack_trace_depth(stackTraceDepth);
		}

		int wallCheckInterval = 0;
		argument = strchr(mode, 'w');
		if (argument != NULL
			&& sscanf(argument, "w%d", &wallCheckInterval) == 1) {
			heap_debug_start_wall_checking(wallCheckInterval);
		}
	}

	return B_OK;
}


extern "C" void
__heap_terminate_after()
{
	if (sCurrentHeap->terminate_after != NULL)
		sCurrentHeap->terminate_after();
}


extern "C" void
__heap_before_fork(void)
{
}


extern "C" void
__heap_after_fork_child(void)
{
}


extern "C" void
__heap_after_fork_parent(void)
{
}


extern "C" void
__heap_thread_init(void)
{
}


extern "C" void
__heap_thread_exit(void)
{
}


// #pragma mark - Public API


extern "C" void*
memalign(size_t alignment, size_t size)
{
	return sCurrentHeap->memalign(alignment, size);
}


extern "C" void*
malloc(size_t size)
{
	return sCurrentHeap->malloc(size);
}


extern "C" void
free(void* address)
{
	sCurrentHeap->free(address);
}


extern "C" void*
realloc(void* address, size_t newSize)
{
	return sCurrentHeap->realloc(address, newSize);
}


extern "C" void*
calloc(size_t numElements, size_t size)
{
	if (sCurrentHeap->calloc != NULL)
		return sCurrentHeap->calloc(numElements, size);

	void* address = malloc(numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}


extern "C" void*
valloc(size_t size)
{
	if (sCurrentHeap->valloc != NULL)
		return sCurrentHeap->valloc(size);

	return memalign(B_PAGE_SIZE, size);
}


extern "C" int
posix_memalign(void **pointer, size_t alignment, size_t size)
{
	if (sCurrentHeap->posix_memalign != NULL)
		return sCurrentHeap->posix_memalign(pointer, alignment, size);

	if (!is_valid_alignment(alignment))
		return EINVAL;

	*pointer = memalign(alignment, size);
	if (*pointer == NULL)
		return ENOMEM;

	return 0;
}
