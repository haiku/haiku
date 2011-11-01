/*
 * Copyright 2011, Michael Lotz <mmlr@mlotz.ch>.
 * Copyright 2011, Ingo Weinhold <ingo_weinhold@gmx.de>.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef SLAB_DEBUG_H
#define SLAB_DEBUG_H


#include <debug.h>
#include <slab/Slab.h>
#include <tracing.h>

#include "kernel_debug_config.h"


//#define TRACE_SLAB
#ifdef TRACE_SLAB
#define TRACE_CACHE(cache, format, args...) \
	dprintf("Cache[%p, %s] " format "\n", cache, cache->name , ##args)
#else
#define TRACE_CACHE(cache, format, bananas...) do { } while (0)
#endif


#define COMPONENT_PARANOIA_LEVEL	OBJECT_CACHE_PARANOIA
#include <debug_paranoia.h>


// Macros determining whether allocation tracking is actually available.
#define SLAB_OBJECT_CACHE_ALLOCATION_TRACKING (SLAB_ALLOCATION_TRACKING != 0 \
	&& SLAB_OBJECT_CACHE_TRACING != 0 \
	&& SLAB_OBJECT_CACHE_TRACING_STACK_TRACE > 0)
	// The object cache code needs to do allocation tracking.
#define SLAB_MEMORY_MANAGER_ALLOCATION_TRACKING (SLAB_ALLOCATION_TRACKING != 0 \
	&& SLAB_MEMORY_MANAGER_TRACING != 0 \
	&& SLAB_MEMORY_MANAGER_TRACING_STACK_TRACE > 0)
	// The memory manager code needs to do allocation tracking.
#define SLAB_ALLOCATION_TRACKING_AVAILABLE \
	(SLAB_OBJECT_CACHE_ALLOCATION_TRACKING \
		|| SLAB_MEMORY_MANAGER_ALLOCATION_TRACKING)
	// Guards code that is needed for either object cache or memory manager
	// allocation tracking.


struct object_depot;


#if SLAB_ALLOCATION_TRACKING_AVAILABLE

class AllocationTrackingInfo {
public:
	AbstractTraceEntryWithStackTrace*	traceEntry;
	bigtime_t							traceEntryTimestamp;

public:
	void Init(AbstractTraceEntryWithStackTrace* entry)
	{
		traceEntry = entry;
		traceEntryTimestamp = entry != NULL ? entry->Time() : -1;
			// Note: this is a race condition, if the tracing buffer wrapped and
			// got overwritten once, we would access an invalid trace entry
			// here. Obviously this is rather unlikely.
	}

	void Clear()
	{
		traceEntry = NULL;
		traceEntryTimestamp = 0;
	}

	bool IsInitialized() const
	{
		return traceEntryTimestamp != 0;
	}

	AbstractTraceEntryWithStackTrace* TraceEntry() const
	{
		return traceEntry;
	}

	bool IsTraceEntryValid() const
	{
		return tracing_is_entry_valid(traceEntry, traceEntryTimestamp);
	}
};


namespace BKernel {

class AllocationTrackingCallback {
public:
	virtual						~AllocationTrackingCallback();

	virtual	bool				ProcessTrackingInfo(
									AllocationTrackingInfo* info,
									void* allocation,
									size_t allocationSize) = 0;
};

}

using BKernel::AllocationTrackingCallback;

#endif // SLAB_ALLOCATION_TRACKING_AVAILABLE


void		dump_object_depot(object_depot* depot);
int			dump_object_depot(int argCount, char** args);
int			dump_depot_magazine(int argCount, char** args);


#if PARANOID_KERNEL_MALLOC || PARANOID_KERNEL_FREE
static inline void*
fill_block(void* buffer, size_t size, uint32 pattern)
{
	if (buffer == NULL)
		return NULL;

	size &= ~(sizeof(pattern) - 1);
	for (size_t i = 0; i < size / sizeof(pattern); i++)
		((uint32*)buffer)[i] = pattern;

	return buffer;
}
#endif


static inline void*
fill_allocated_block(void* buffer, size_t size)
{
#if PARANOID_KERNEL_MALLOC
	return fill_block(buffer, size, 0xcccccccc);
#else
	return buffer;
#endif
}


static inline void*
fill_freed_block(void* buffer, size_t size)
{
#if PARANOID_KERNEL_FREE
	return fill_block(buffer, size, 0xdeadbeef);
#else
	return buffer;
#endif
}


#endif	// SLAB_DEBUG_H
