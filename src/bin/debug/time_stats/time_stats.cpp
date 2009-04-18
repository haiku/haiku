/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>

#include <OS.h>

#include <AutoDeleter.h>

#include <scheduler_defs.h>
#include <syscalls.h>
#include <thread_defs.h>


#define MAX_THREADS	4096

#define SCHEDULING_ANALYSIS_BUFFER_SIZE	10 * 1024 * 1024


extern const char* __progname;

static const char* kUsage =
	"Usage: %s [ <options> ] <command line>\n"
	"Executes the given command line <command line> and print an analysis of\n"
	"the user and kernel times of all threads that ran during that time.\n"
	"\n"
	"Options:\n"
	"  -b <size>    - When doing scheduling analysis: the size of the buffer\n"
	"                 used (in MB)\n"
	"  -h, --help   - Print this usage info.\n"
	"  -o <output>  - Print the results to file <output>.\n"
	"  -s           - Also perform a scheduling analysis over the time the\n"
	"                 child process ran. This requires that scheduler kernel\n"
	"                 tracing had been enabled at compile time.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, __progname);
    exit(error ? 1 : 0);
}


struct UsageInfoThreadComparator {
	inline bool operator()(const thread_info& a, const thread_info& b)
	{
		return a.thread < b.thread;
	}
};


struct UsageInfoTimeComparator {
	inline bool operator()(const thread_info& a, const thread_info& b)
	{
		return a.user_time + a.kernel_time > b.user_time + b.kernel_time;
	}
};


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


static int32
get_usage_infos(thread_info* infos)
{
	int32 count = 0;

	int32 teamCookie = 0;
	team_info teamInfo;
	while (get_next_team_info(&teamCookie, &teamInfo) == B_OK) {
		int32 threadCookie = 0;
		while (get_next_thread_info(teamInfo.team, &threadCookie, &infos[count])
				== B_OK) {
			count++;
		}
	}

	return count;
}


static pid_t
run_child(int argc, const char* const* argv)
{
	// fork
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "Error: fork() failed: %s\n", strerror(errno));
		exit(1);
	}

	// exec child process
	if (child == 0) {
		execvp(argv[0], (char**)argv);
		exit(1);
	}

	// wait for child
	int childStatus;
	while (wait(&childStatus) < 0);

	return child;
}


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
		case THREAD_BLOCK_TYPE_OTHER:
			sprintf(buffer, "other %p (%s)", object, waitObject->name);
			break;
		default:
			sprintf(buffer, "unknown %p", object);
			break;
	}

	return buffer;
}


static void
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


static void
do_timing_analysis(int argc, const char* const* argv, bool schedulingAnalysis,
	int outFD, size_t bufferSize)
{
	// gather initial usage info
	thread_info initialUsage[MAX_THREADS];
	int32 initialUsageCount = get_usage_infos(initialUsage);

	// exec child process
	bigtime_t startTime = system_time();
	pid_t child = run_child(argc, argv);

	// get child usage info
	bigtime_t endTime = system_time();
	bigtime_t runTime = endTime - startTime;
	team_usage_info childUsage;
	get_team_usage_info(B_CURRENT_TEAM, B_TEAM_USAGE_CHILDREN, &childUsage);

	// gather final usage info
	thread_info finalUsage[MAX_THREADS];
	int32 finalUsageCount = get_usage_infos(finalUsage);

	// sort the infos, so we can compare them better
	std::sort(initialUsage, initialUsage + initialUsageCount,
		UsageInfoThreadComparator());
	std::sort(finalUsage, finalUsage + finalUsageCount,
		UsageInfoThreadComparator());

	// compute results

	thread_info sortedThreads[MAX_THREADS];
	int32 sortedThreadCount = 0;
	thread_info goneThreads[MAX_THREADS];
	int32 goneThreadCount = 0;

	// child
	sortedThreads[0].thread = child;
	sortedThreads[0].user_time = childUsage.user_time;
	sortedThreads[0].kernel_time = childUsage.kernel_time;
	strlcpy(sortedThreads[0].name, "<child>", sizeof(sortedThreads[0].name));
	sortedThreadCount++;

	// other threads
	int32 initialI = 0;
	int32 finalI = 0;
	while (initialI < initialUsageCount || finalI < finalUsageCount) {
		if (initialI >= initialUsageCount
			|| (finalI < finalUsageCount
				&& initialUsage[initialI].thread > finalUsage[finalI].thread)) {
			// new thread
			memcpy(&sortedThreads[sortedThreadCount], &finalUsage[finalI],
				sizeof(thread_info));
			sortedThreadCount++;
			finalI++;
			continue;
		}

		if (finalI >= finalUsageCount
			|| (initialI < initialUsageCount
				&& initialUsage[initialI].thread < finalUsage[finalI].thread)) {
			// gone thread
			memcpy(&goneThreads[goneThreadCount], &initialUsage[initialI],
				sizeof(thread_info));
			goneThreadCount++;
			initialI++;
			continue;
		}

		// thread is still there
		memcpy(&sortedThreads[sortedThreadCount], &finalUsage[finalI],
			sizeof(thread_info));
		sortedThreads[sortedThreadCount].user_time
			-= initialUsage[initialI].user_time;
		sortedThreads[sortedThreadCount].kernel_time
			-= initialUsage[initialI].kernel_time;
		sortedThreadCount++;
		initialI++;
		finalI++;
	}

	// sort results
	std::sort(sortedThreads, sortedThreads + sortedThreadCount,
		UsageInfoTimeComparator());

	// redirect output, if requested
	if (outFD >= 0)
		dup2(outFD, STDOUT_FILENO);

	// print results
	printf("\nTotal run time: %lld us\n", runTime);
	printf("Thread time statistics in us:\n\n");
	printf(" thread  name                                  kernel        user  "
		"     total    in %%\n");
	printf("-------------------------------------------------------------------"
		"------------------\n");

	for (int32 i = 0; i < sortedThreadCount; i++) {
		const thread_info& info = sortedThreads[i];
		if (info.user_time + info.kernel_time == 0)
			break;

		bool highlight = info.thread == child && isatty(STDOUT_FILENO);

		if (highlight)
			printf("\033[1m");

		bigtime_t total = info.user_time + info.kernel_time;
		printf("%7ld  %-32s  %10lld  %10lld  %10lld  %6.2f\n", info.thread,
			info.name, info.kernel_time, info.user_time, total,
			(double)total / runTime * 100);

		if (highlight)
			printf("\033[m");
	}

	for (int32 i = 0; i < goneThreadCount; i++) {
		const thread_info& info = goneThreads[i];
		printf("%7ld  %-32s  <gone>\n", info.thread, info.name);
	}

	if (schedulingAnalysis)
		do_scheduling_analysis(startTime, endTime, bufferSize);
}


int
main(int argc, const char* const* argv)
{
	const char* outputFile = NULL;
	bool schedulingAnalysis = false;
	size_t bufferSize = SCHEDULING_ANALYSIS_BUFFER_SIZE;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "output", required_argument, 0, 'o' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+b:ho:s", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'b':
				bufferSize = atol(optarg);
				if (bufferSize < 1 || bufferSize > 1024) {
					fprintf(stderr, "Error: Invalid buffer size. Should be "
						"between 1 and 1024 MB\n");
					exit(1);
				}
				bufferSize *= 1024 * 1024;
				break;
			case 'h':
				print_usage_and_exit(false);
				break;
			case 'o':
				outputFile = optarg;
				break;
			case 's':
				schedulingAnalysis = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind >= argc)
		print_usage_and_exit(true);

	// open output file, if specified
	int outFD = -1;
	if (outputFile != NULL) {
		outFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP| S_IROTH | S_IWOTH);
		if (outFD < 0) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s\n", outputFile,
				strerror(errno));
			exit(1);
		}
	}

	do_timing_analysis(argc - optind, argv + optind, schedulingAnalysis, outFD,
		bufferSize);

	return 0;
}
