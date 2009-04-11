/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SYSTEM_PROFILER_DEFS_H
#define _SYSTEM_SYSTEM_PROFILER_DEFS_H

#include <image.h>


// events
enum {
	B_SYSTEM_PROFILER_TEAM_ADDED = 0,
	B_SYSTEM_PROFILER_TEAM_REMOVED,
	B_SYSTEM_PROFILER_TEAM_EXEC,
	B_SYSTEM_PROFILER_THREAD_ADDED,
	B_SYSTEM_PROFILER_THREAD_REMOVED,
	B_SYSTEM_PROFILER_IMAGE_ADDED,
	B_SYSTEM_PROFILER_IMAGE_REMOVED,
	B_SYSTEM_PROFILER_SAMPLES,
	B_SYSTEM_PROFILER_SAMPLES_END
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
};

// B_SYSTEM_PROFILER_TEAM_REMOVED
struct system_profiler_team_removed {
	team_id		team;
};

// B_SYSTEM_PROFILER_TEAM_EXEC
struct system_profiler_team_exec {
	team_id		team;
};

// B_SYSTEM_PROFILER_THREAD_ADDED
struct system_profiler_thread_added {
	team_id		team;
	thread_id	thread;
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


#endif	/* _SYSTEM_SYSTEM_PROFILER_DEFS_H */
