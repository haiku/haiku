/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <OS.h>

#include <AutoDeleter.h>

#include <scheduler_defs.h>
#include <syscalls.h>
#include <thread_defs.h>

#include "time_stats.h"


struct wait_object_group {
	scheduling_analysis_thread_wait_object**	objects;
	int32										count;
	bigtime_t									wait_time;
	int64										waits;
};


struct ThreadRunTimeComparator {
	inline bool operator()(const scheduling_analysis_thread* a,
		const scheduling_analysis_thread* b)
	{
		return a->total_run_time > b->total_run_time;
	}
};


struct WaitObjectGroupingComparator {
	inline bool operator()(const scheduling_analysis_thread_wait_object* a,
		const scheduling_analysis_thread_wait_object* b)
	{
		return a->wait_object->type < b->wait_object->type
			|| (a->wait_object->type == b->wait_object->type
				&& strcmp(a->wait_object->name, b->wait_object->name) < 0);
	}
};


struct WaitObjectTimeComparator {
	inline bool operator()(const scheduling_analysis_thread_wait_object* a,
		const scheduling_analysis_thread_wait_object* b)
	{
		return a->wait_time > b->wait_time;
	}
};


struct WaitObjectGroupTimeComparator {
	inline bool operator()(const wait_object_group& a,
		const wait_object_group& b)
	{
		return a.wait_time > b.wait_time;
	}
};


static const char*
wait_object_to_string(scheduling_analysis_wait_object* waitObject, char* buffer,
	bool nameOnly = false)
{
	uint32 type = waitObject->type;
	void* object = waitObject->object;

	switch (type) {
		case THREAD_BLOCK_TYPE_SEMAPHORE:
			if (nameOnly) {
				sprintf(buffer, "sem \"%s\"", waitObject->name);
			} else {
				sprintf(buffer, "sem %ld (%s)", (sem_id)(addr_t)object,
					waitObject->name);
			}
			break;
		case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
			if (nameOnly) {
				sprintf(buffer, "cvar \"%s\"", waitObject->name);
			} else {
				sprintf(buffer, "cvar %p (%s %p)", object, waitObject->name,
					waitObject->referenced_object);
			}
			break;
		case THREAD_BLOCK_TYPE_SNOOZE:
			strcpy(buffer, "snooze");
			break;
		case THREAD_BLOCK_TYPE_SIGNAL:
			strcpy(buffer, "signal");
			break;
		case THREAD_BLOCK_TYPE_MUTEX:
			if (nameOnly)
				sprintf(buffer, "mutex \"%s\"", waitObject->name);
			else
				sprintf(buffer, "mutex %p (%s)", object, waitObject->name);
			break;
		case THREAD_BLOCK_TYPE_RW_LOCK:
			if (nameOnly)
				sprintf(buffer, "rwlock \"%s\"", waitObject->name);
			else
				sprintf(buffer, "rwlock %p (%s)", object, waitObject->name);
			break;
		case THREAD_BLOCK_TYPE_USER:
			strcpy(buffer, "user");
			break;
		case THREAD_BLOCK_TYPE_OTHER:
			sprintf(buffer, "other %p (%s)", object, waitObject->name);
			break;
		default:
			sprintf(buffer, "unknown %p", object);
			break;
	}

	return buffer;
}


void
do_scheduling_analysis(bigtime_t startTime, bigtime_t endTime,
	size_t bufferSize)
{
	printf("\n");

	// allocate a chunk of memory for the scheduling analysis
	void* buffer = malloc(bufferSize);
	if (buffer == NULL) {
		fprintf(stderr, "Error: Failed to allocate memory for the scheduling "
			"analysis.\n");
		exit(1);
	}
	MemoryDeleter _(buffer);

	// do the scheduling analysis
	scheduling_analysis analysis;
	status_t error = _kern_analyze_scheduling(startTime, endTime, buffer,
		bufferSize, &analysis);
	if (error != B_OK) {
		fprintf(stderr, "Error: Scheduling analysis failed: %s\n",
			strerror(error));
		exit(1);
	}

	// allocate arrays for grouping and sorting the wait objects
	scheduling_analysis_thread_wait_object** waitObjects
		= new(std::nothrow) scheduling_analysis_thread_wait_object*[
			analysis.thread_wait_object_count];
	ArrayDeleter<scheduling_analysis_thread_wait_object*> _2(waitObjects);

	wait_object_group* waitObjectGroups = new(std::nothrow) wait_object_group[
		analysis.thread_wait_object_count];
	ArrayDeleter<wait_object_group> _3(waitObjectGroups);

	if (waitObjects == NULL || waitObjectGroups == NULL) {
		fprintf(stderr, "Error: Out of memory\n");
		exit(1);
	}

	printf("scheduling analysis: %lu threads, %llu wait objects, "
		"%llu thread wait objects\n", analysis.thread_count,
		analysis.wait_object_count, analysis.thread_wait_object_count);

	// sort the thread by run time
	std::sort(analysis.threads, analysis.threads + analysis.thread_count,
		ThreadRunTimeComparator());

	for (uint32 i = 0; i < analysis.thread_count; i++) {
		scheduling_analysis_thread* thread = analysis.threads[i];

		// compute total wait time and prepare the objects for sorting
		int32 waitObjectCount = 0;
		bigtime_t waitTime = 0;
		scheduling_analysis_thread_wait_object* threadWaitObject
			= thread->wait_objects;
		while (threadWaitObject != NULL) {
			waitObjects[waitObjectCount++] = threadWaitObject;
			waitTime += threadWaitObject->wait_time;
			threadWaitObject = threadWaitObject->next_in_list;
		}

		// sort the wait objects by type + name
		std::sort(waitObjects, waitObjects + waitObjectCount,
			WaitObjectGroupingComparator());

		// create the groups
		wait_object_group* group = NULL;
		int32 groupCount = 0;
		for (int32 i = 0; i < waitObjectCount; i++) {
			scheduling_analysis_thread_wait_object* threadWaitObject
				= waitObjects[i];
			scheduling_analysis_wait_object* waitObject
				= threadWaitObject->wait_object;

			if (groupCount == 0 || strcmp(waitObject->name, "?") == 0
				|| waitObject->type != group->objects[0]->wait_object->type
				|| strcmp(waitObject->name,
						group->objects[0]->wait_object->name) != 0) {
				// create a new group
				group = &waitObjectGroups[groupCount++];
				group->objects = waitObjects + i;
				group->count = 0;
				group->wait_time = 0;
				group->waits = 0;
			}

			group->count++;
			group->wait_time += threadWaitObject->wait_time;
			group->waits += threadWaitObject->waits;
		}

		// sort the groups by wait time
		std::sort(waitObjectGroups, waitObjectGroups + groupCount,
			WaitObjectGroupTimeComparator());

		printf("\nthread %ld \"%s\":\n", thread->id, thread->name);
		printf("  run time:    %lld us (%lld runs)\n", thread->total_run_time,
			thread->runs);
		printf("  wait time:   %lld us\n", waitTime);
		printf("  latencies:   %lld us (%lld)\n", thread->total_latency,
			thread->latencies);
		printf("  preemptions: %lld us (%lld)\n", thread->total_rerun_time,
			thread->reruns);
		printf("  unspecified: %lld us\n", thread->unspecified_wait_time);

		printf("  waited on:\n");
		for (int32 i = 0; i < groupCount; i++) {
			wait_object_group& group = waitObjectGroups[i];
			char buffer[1024];

			if (group.count == 1) {
				// only one element -- just print it
				scheduling_analysis_thread_wait_object* threadWaitObject
					= group.objects[0];
				scheduling_analysis_wait_object* waitObject
					= threadWaitObject->wait_object;
				wait_object_to_string(waitObject, buffer);
				printf("    %s: %lld us (%lld)\n", buffer,
					threadWaitObject->wait_time, threadWaitObject->waits);
			} else {
				// sort the wait objects by wait time
				std::sort(group.objects, group.objects + group.count,
					WaitObjectTimeComparator());

				// print the group line
				wait_object_to_string(group.objects[0]->wait_object, buffer,
					true);
				printf("    group %s: %lld us (%lld)\n", buffer,
					group.wait_time, group.waits);

				// print the wait objects
				for (int32 k = 0; k < group.count; k++) {
					scheduling_analysis_thread_wait_object* threadWaitObject
						= group.objects[k];
					scheduling_analysis_wait_object* waitObject
						= threadWaitObject->wait_object;
					wait_object_to_string(waitObject, buffer);
					printf("      %s: %lld us (%lld)\n", buffer,
						threadWaitObject->wait_time, threadWaitObject->waits);
				}
			}
		}
	}
}
