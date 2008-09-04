/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SCHEDULER_DEFS_H
#define _SYSTEM_SCHEDULER_DEFS_H

#include <OS.h>


struct scheduling_analysis_thread_wait_object;

struct scheduling_analysis_thread {
	thread_id	id;
	char		name[B_OS_NAME_LENGTH];

	int64		runs;
	bigtime_t	total_run_time;
	bigtime_t	min_run_time;
	bigtime_t	max_run_time;

	int64		latencies;
	bigtime_t	total_latency;
	bigtime_t	min_latency;
	bigtime_t	max_latency;

	int64		reruns;
	bigtime_t	total_rerun_time;
	bigtime_t	min_rerun_time;
	bigtime_t	max_rerun_time;

	bigtime_t	unspecified_wait_time;

	int64		preemptions;

	scheduling_analysis_thread_wait_object* wait_objects;
};


struct scheduling_analysis_wait_object {
	uint32		type;
	void*		object;
	char		name[B_OS_NAME_LENGTH];
	void*		referenced_object;
};


struct scheduling_analysis_thread_wait_object {
	thread_id								thread;
	scheduling_analysis_wait_object*		wait_object;
	bigtime_t								wait_time;
	int64									waits;
	scheduling_analysis_thread_wait_object*	next_in_list;
};


struct scheduling_analysis {
	uint32							thread_count;
	scheduling_analysis_thread**	threads;
	uint64							wait_object_count;
	uint64							thread_wait_object_count;
};


#endif	/* _SYSTEM_SCHEDULER_DEFS_H */
