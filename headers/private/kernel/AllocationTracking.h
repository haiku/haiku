/*
 * Copyright 2011, Michael Lotz <mmlr@mlotz.ch>.
 * Copyright 2011, Ingo Weinhold <ingo_weinhold@gmx.de>.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef ALLOCATION_TRACKING_H
#define ALLOCATION_TRACKING_H


#include <debug.h>
#include <tracing.h>


namespace BKernel {

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

}	// namespace BKernel


using BKernel::AllocationTrackingInfo;


#endif	// ALLOCATION_TRACKING_H
