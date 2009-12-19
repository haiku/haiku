/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SYSTEM_PROFILER_DEFS_H
#define _SYSTEM_SYSTEM_PROFILER_DEFS_H


#include <image.h>


struct system_profiler_parameters {
	// general
	area_id		buffer_area;			// area the events will be written to
	uint32		flags;					// flags selecting the events to receive

	// scheduling
	size_t		locking_lookup_size;	// size of the lookup table used for
										// caching the locking primitive infos

	// sampling
	bigtime_t	interval;				// interval at which to take samples
	uint32		stack_depth;			// maximum stack depth to sample
};


// event flags
enum {
	B_SYSTEM_PROFILER_TEAM_EVENTS			= 0x01,
	B_SYSTEM_PROFILER_THREAD_EVENTS			= 0x02,
	B_SYSTEM_PROFILER_IMAGE_EVENTS			= 0x04,
	B_SYSTEM_PROFILER_SAMPLING_EVENTS		= 0x08,
	B_SYSTEM_PROFILER_SCHEDULING_EVENTS		= 0x10,
	B_SYSTEM_PROFILER_IO_SCHEDULING_EVENTS	= 0x20
};


// events
enum {
	// reserved for the user application
	B_SYSTEM_PROFILER_USER_EVENT = 0,

	// ring buffer wrap-around marker
	B_SYSTEM_PROFILER_BUFFER_END,

	// team
	B_SYSTEM_PROFILER_TEAM_ADDED,
	B_SYSTEM_PROFILER_TEAM_REMOVED,
	B_SYSTEM_PROFILER_TEAM_EXEC,

	// thread
	B_SYSTEM_PROFILER_THREAD_ADDED,
	B_SYSTEM_PROFILER_THREAD_REMOVED,

	// image
	B_SYSTEM_PROFILER_IMAGE_ADDED,
	B_SYSTEM_PROFILER_IMAGE_REMOVED,

	// profiling samples
	B_SYSTEM_PROFILER_SAMPLES,

	// scheduling
	B_SYSTEM_PROFILER_THREAD_SCHEDULED,
	B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE,
	B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE,
	B_SYSTEM_PROFILER_WAIT_OBJECT_INFO,

	// I/O scheduling
	B_SYSTEM_PROFILER_IO_SCHEDULER_ADDED,
	B_SYSTEM_PROFILER_IO_SCHEDULER_REMOVED,
	B_SYSTEM_PROFILER_IO_REQUEST_SCHEDULED,
	B_SYSTEM_PROFILER_IO_REQUEST_FINISHED,
	B_SYSTEM_PROFILER_IO_OPERATION_STARTED,
	B_SYSTEM_PROFILER_IO_OPERATION_FINISHED
};


struct system_profiler_buffer_header {
	size_t	start;
	size_t	size;
};


struct system_profiler_event_header {
	uint8	event;
	uint8	cpu;	// only for B_SYSTEM_PROFILER_SAMPLES
	uint16	size;	// size of the event structure excluding the header
};


// B_SYSTEM_PROFILER_TEAM_ADDED
struct system_profiler_team_added {
	team_id		team;
	uint16		args_offset;
	char		name[1];
};

// B_SYSTEM_PROFILER_TEAM_REMOVED
struct system_profiler_team_removed {
	team_id		team;
};

// B_SYSTEM_PROFILER_TEAM_EXEC
struct system_profiler_team_exec {
	team_id		team;
	char		thread_name[B_OS_NAME_LENGTH];
	char		args[1];
};

// B_SYSTEM_PROFILER_THREAD_ADDED
struct system_profiler_thread_added {
	team_id		team;
	thread_id	thread;
	char		name[B_OS_NAME_LENGTH];
};

// B_SYSTEM_PROFILER_THREAD_REMOVED
struct system_profiler_thread_removed {
	team_id		team;
	thread_id	thread;
};

// B_SYSTEM_PROFILER_IMAGE_ADDED
struct system_profiler_image_added {
	team_id		team;
	image_info	info;
};

// B_SYSTEM_PROFILER_IMAGE_REMOVED
struct system_profiler_image_removed {
	team_id		team;
	image_id	image;
};

// B_SYSTEM_PROFILER_SAMPLES
struct system_profiler_samples {
	thread_id	thread;
	addr_t		samples[0];
};

// base structure for the following three
struct system_profiler_thread_scheduling_event {
	nanotime_t	time;
	thread_id	thread;
};

// B_SYSTEM_PROFILER_THREAD_SCHEDULED
struct system_profiler_thread_scheduled {
	nanotime_t	time;
	thread_id	thread;
	thread_id	previous_thread;
	uint16		previous_thread_state;
	uint16		previous_thread_wait_object_type;
	addr_t		previous_thread_wait_object;
};

// B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE
struct system_profiler_thread_enqueued_in_run_queue {
	nanotime_t	time;
	thread_id	thread;
	uint8		priority;
};

// B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE
struct system_profiler_thread_removed_from_run_queue {
	nanotime_t	time;
	thread_id	thread;
};

// B_SYSTEM_PROFILER_WAIT_OBJECT_INFO
struct system_profiler_wait_object_info {
	uint32		type;
	addr_t		object;
	addr_t		referenced_object;
	char		name[1];
};

// B_SYSTEM_PROFILER_IO_SCHEDULER_ADDED
struct system_profiler_io_scheduler_added {
	int32		scheduler;
	char		name[1];
};

// B_SYSTEM_PROFILER_IO_SCHEDULER_REMOVED
struct system_profiler_io_scheduler_removed {
	int32		scheduler;
};

// B_SYSTEM_PROFILER_IO_REQUEST_SCHEDULED
struct system_profiler_io_request_scheduled {
	nanotime_t	time;
	int32		scheduler;
	team_id		team;
	thread_id	thread;
	void*		request;
	off_t		offset;
	size_t		length;
	bool		write;
	uint8		priority;
};

// B_SYSTEM_PROFILER_IO_REQUEST_FINISHED
struct system_profiler_io_request_finished {
	nanotime_t	time;
	int32		scheduler;
	void*		request;
	status_t	status;
	size_t		transferred;
};

// B_SYSTEM_PROFILER_IO_OPERATION_STARTED
struct system_profiler_io_operation_started {
	nanotime_t	time;
	int32		scheduler;
	void*		request;
	void*		operation;
	off_t		offset;
	size_t		length;
	bool		write;
};

// B_SYSTEM_PROFILER_IO_OPERATION_FINISHED
struct system_profiler_io_operation_finished {
	nanotime_t	time;
	int32		scheduler;
	void*		request;
	void*		operation;
	status_t	status;
	size_t		transferred;
};


#endif	/* _SYSTEM_SYSTEM_PROFILER_DEFS_H */
