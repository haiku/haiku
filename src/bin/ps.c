/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Francois Revol (mmu_man)
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */
#include <stdio.h>
#include <string.h>

#include <OS.h>

#define SNOOZE_TIME 100000


int
main(int argc, char **argv)
{
	team_info teamInfo;
	thread_info threadInfo;
	uint32 teamCookie = 0;
	uint32 threadCookie = 0;
	sem_info semaphoreInfo;
	char *threadState;
	char *states[] = {"run", "rdy", "msg", "zzz", "sus", "wait" };
	system_info systemInfo;
	bool printSystemInfo = true;
	// match this in team name
	char *string_to_match;

	// TODO: parse command line
	// Possible command line options:
	// 		-h	show help
	// 			ps			- by default it only shows all team info
	// 			ps [team]	- shows info about this team with all its threads
	// 		-t	pstree like output
	// 		-a	show threads too (by default only teams are displayed)
	// 		-s	show semaphore info
	// 		-i	show system info
	string_to_match = (argc == 2) ? argv[1] : NULL;

	printf("%-50s %4s %8s %4s %4s\n", "Team", "Id", "#Threads", "Gid", "Uid");
	while (get_next_team_info(&teamCookie, &teamInfo) >= B_OK) {
		if (string_to_match) {
			char *p;
			p = teamInfo.args;
			if ((p = strchr(p, ' ')))
				*p = '\0'; /* remove arguments, keep only argv[0] */
			p = strrchr(teamInfo.args, '/'); /* forget the path */
			if (p == NULL)
				p = teamInfo.args;
			if (strstr(p, string_to_match) == NULL)
				continue;
			// Print team info
			printf("%-50s %4ld %8ld %4d %4d\n\n", teamInfo.args, teamInfo.team,
				teamInfo.thread_count, teamInfo.uid, teamInfo.gid);

			printf("%-30s %4s %8s %4s %8s %8s\n", "Thread", "Id", "State",
				"Prio", "UTime", "KTime");
			// Print all info about its threads too
			while (get_next_thread_info(teamInfo.team, &threadCookie, &threadInfo)
				>= B_OK) {
				if (threadInfo.state < B_THREAD_RUNNING
					|| threadInfo.state > B_THREAD_WAITING)
					// This should never happen
					threadState = "???";
				else
					threadState = states[threadInfo.state - 1];

				printf("%-30s %4ld %8s %4ld %8llu %8llu ",
					threadInfo.name, threadInfo.thread, threadState,
					threadInfo.priority, (threadInfo.user_time / 1000),
					(threadInfo.kernel_time / 1000));

				if (threadInfo.state == B_THREAD_WAITING && threadInfo.sem != -1) {
					status_t status = get_sem_info(threadInfo.sem, &semaphoreInfo);
					if (status == B_OK)
						printf("%s(%ld)\n", semaphoreInfo.name, semaphoreInfo.sem);
					else
						printf("%s(%ld)\n", strerror(status), threadInfo.sem);
				} else
					puts("");
			}
			break;

		} else {
			printf("%-50s %4ld %8ld %4d %4d\n", teamInfo.args, teamInfo.team,
				teamInfo.thread_count, teamInfo.uid, teamInfo.gid);
		}
	}

	if (printSystemInfo) {
		// system stats
		get_system_info(&systemInfo);
		printf("\nSystem Info\n");
		printf("%luk (%lu bytes) total memory\n",
			(systemInfo.max_pages * B_PAGE_SIZE / 1024),
			(systemInfo.max_pages * B_PAGE_SIZE));
		printf("%luk (%lu bytes) currently committed\n",
			(systemInfo.used_pages * B_PAGE_SIZE / 1024),
			(systemInfo.used_pages * B_PAGE_SIZE));
		printf("%luk (%lu bytes) currently available\n",
			(systemInfo.max_pages - systemInfo.used_pages) * B_PAGE_SIZE / 1024,
			(systemInfo.max_pages - systemInfo.used_pages) * B_PAGE_SIZE);
		printf("%2.1f%% memory utilisation\n",
			(float)100 * systemInfo.used_pages / systemInfo.max_pages);
	}
	return 0;
}
