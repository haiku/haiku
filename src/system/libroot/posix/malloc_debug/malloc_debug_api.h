/*
 * Copyright 2015, Michael Lotz <mmlr@mlotz.ch>.
 * Distributed under the terms of the MIT License.
 */
#ifndef MALLOC_DEBUG_API_H
#define MALLOC_DEBUG_API_H

#include <OS.h>


struct heap_implementation {
	status_t	(*init)();
	void		(*terminate_after)();

	// Mandatory hooks
	void*		(*memalign)(size_t alignment, size_t size);
	void*		(*malloc)(size_t size);
	void		(*free)(void* address);
	void*		(*realloc)(void* address, size_t newSize);

	// Hooks with default implementations
	void*		(*calloc)(size_t numElements, size_t size);
	void*		(*valloc)(size_t size);
	int			(*posix_memalign)(void** pointer, size_t alignment,
					size_t size);

	// Heap Debug API
	status_t	(*start_wall_checking)(int msInterval);
	status_t	(*stop_wall_checking)();
	void		(*set_paranoid_validation)(bool enabled);
	void		(*set_memory_reuse)(bool enabled);
	void		(*set_debugger_calls)(bool enabled);
	void		(*set_default_alignment)(size_t defaultAlignment);
	void		(*validate_heaps)();
	void		(*validate_walls)();
	void		(*dump_allocations)(bool statsOnly, thread_id thread);
	void		(*dump_heaps)(bool dumpAreas, bool dumpBins);
	void*		(*malloc_with_guard_page)(size_t size);
	status_t	(*get_allocation_info)(void* address, size_t *size,
					thread_id *thread);
	status_t	(*set_dump_allocations_on_exit)(bool enabled);
	status_t	(*set_stack_trace_depth)(size_t stackTraceDepth);
};


extern heap_implementation __mallocDebugHeap;
extern heap_implementation __mallocGuardedHeap;


static inline bool
is_valid_alignment(size_t number)
{
	// this cryptic line accepts zero and all powers of two
	return ((~number + 1) | ((number << 1) - 1)) == ~0UL;
}

#endif // MALLOC_DEBUG_API_H
