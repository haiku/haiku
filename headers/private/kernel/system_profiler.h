/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SYSTEM_PROFILER_H
#define _KERNEL_SYSTEM_PROFILER_H

#include <sys/cdefs.h>

#include <OS.h>


struct system_profiler_parameters;


__BEGIN_DECLS

status_t start_system_profiler(size_t areaSize, uint32 stackDepth,
			bigtime_t interval);
void stop_system_profiler();

status_t _user_system_profiler_start(
			struct system_profiler_parameters* parameters);
status_t _user_system_profiler_next_buffer(size_t bytesRead,
			uint64* _droppedEvents);
status_t _user_system_profiler_stop();
status_t _user_system_profiler_recorded(
			struct system_profiler_parameters* parameters);

__END_DECLS


#endif	/* _KERNEL_SYSTEM_PROFILER_H */
