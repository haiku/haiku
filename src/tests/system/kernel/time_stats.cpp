/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>

#include <OS.h>


#define MAX_THREADS	4096


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


int
main(int argc, const char* const* argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage:...\n");
		exit(1);
	}

	// gather initial usage info
	thread_info initialUsage[MAX_THREADS];
	int32 initialUsageCount = get_usage_infos(initialUsage);

	// fork
	bigtime_t startTime = system_time();
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "fork() failed: %s\n", strerror(errno));
		exit(1);
	}

	// exec child process
	if (child == 0) {
		execvp(argv[1], (char**)(argv + 1));
		exit(1);
	}

	// wait for child
	int childStatus;
	while (wait(&childStatus) < 0);

	// get child usage info
	bigtime_t runTime = system_time() - startTime;
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
			|| finalI < finalUsageCount
				&& initialUsage[initialI].thread > finalUsage[finalI].thread) {
			// new thread
			memcpy(&sortedThreads[sortedThreadCount], &finalUsage[finalI],
				sizeof(thread_info));
			sortedThreadCount++;
			finalI++;
			continue;
		}

		if (finalI >= finalUsageCount
			|| initialI < initialUsageCount
				&& initialUsage[initialI].thread < finalUsage[finalI].thread) {
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

	// print results
	printf("\nTotal run time: %lld us\n", runTime);
	printf("Thread time statistics in us:\n\n");
	printf(" thread  name                                  kernel        user  "
		"     total    in %\n");
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
			info.name, info.user_time, info.kernel_time, total,
			(double)total / runTime * 100);

		if (highlight)
			printf("\033[m");
	}

	for (int32 i = 0; i < goneThreadCount; i++) {
		const thread_info& info = goneThreads[i];
		printf("%7ld  %-32s  <gone>\n", info.thread, info.name);
	}

	return 0;
}
