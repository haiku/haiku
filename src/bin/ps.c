/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Francois Revol (mmu_man)
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 *		Bjoern Herzig (xRaich[o]2x)
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <OS.h>

#define SNOOZE_TIME 100000

char *sStates[] = {"run", "rdy", "msg", "zzz", "sus", "wait" };

static void printTeamThreads(team_info *teamInfo, bool printSemaphoreInfo); 
static void printTeamInfo(team_info *teamInfo, bool printHeader);

static void
printTeamInfo(team_info *teamInfo, bool printHeader)
{
	// Print team info
	if (printHeader)
		printf("%-50s %5s %8s %4s %4s\n", "Team", "Id", "#Threads", "Gid", \
			"Uid");
		
	printf("%-50s %5" B_PRId32 " %8" B_PRId32 " %4d %4d\n", teamInfo->args,
		teamInfo->team, teamInfo->thread_count, teamInfo->uid, teamInfo->gid);
}


static void
printTeamThreads(team_info *teamInfo, bool printSemaphoreInfo)
{
	char *threadState;
	int32 threadCookie = 0;
	sem_info semaphoreInfo;
	thread_info threadInfo;
	
	// Print all info about its threads too
	while (get_next_thread_info(teamInfo->team, &threadCookie, &threadInfo)
		>= B_OK) {
		if (threadInfo.state < B_THREAD_RUNNING
			|| threadInfo.state > B_THREAD_WAITING)
			// This should never happen
			threadState = "???";
		else
			threadState = sStates[threadInfo.state - 1];

		printf("%-37s %5" B_PRId32 " %8s %4" B_PRId32 " %8" B_PRIu64 " %8"
			B_PRId64 " ", threadInfo.name, threadInfo.thread, threadState,
			threadInfo.priority, (threadInfo.user_time / 1000),
			(threadInfo.kernel_time / 1000));

		if (printSemaphoreInfo) {
			if (threadInfo.state == B_THREAD_WAITING && threadInfo.sem != -1) {
				status_t status = get_sem_info(threadInfo.sem, &semaphoreInfo);
				if (status == B_OK) {
					printf("%s(%" B_PRId32 ")\n", semaphoreInfo.name,
						semaphoreInfo.sem);
				} else {
					printf("%s(%" B_PRId32 ")\n", strerror(status),
						threadInfo.sem);
				}
			} else
				puts("");
		} else
			puts("");
	}
}

int
main(int argc, char **argv)
{
	team_info teamInfo;
	int32 teamCookie = 0;
	system_info systemInfo;
	bool printSystemInfo = false;
	bool printThreads = false;
	bool printHeader = true;
	bool printSemaphoreInfo = false;
	// match this in team name
	char *string_to_match;
	
	int c;
	
	while ((c = getopt(argc, argv,"ihas")) != EOF) {
		switch(c) {
			case 'i':
				printSystemInfo = true;
				break;
			case 'h':
				printf( "usage: ps [-hais] [team]\n"
			   			"-h : show help\n"
			   			"-i : show system info\n"
			   			"-s : show semaphore info\n"
			   			"-a : show threads too (by default only teams are " \
			   				"displayed)\n");
				return 0;
				break;
			case 'a':
				printThreads = true;
				break;
			case 's':
				printSemaphoreInfo = true;
				break;
		}
	}

	// TODO: parse command line
	// Possible command line options:
	// 		-t	pstree like output
	
	if (argc == 2 && (printSystemInfo||printThreads))
		string_to_match = NULL;
	else
		string_to_match = (argc >= 2) ? argv[argc-1] : NULL;
	
	if (!string_to_match) {
		while (get_next_team_info(&teamCookie, &teamInfo) >= B_OK) {
			
			printTeamInfo(&teamInfo,printHeader);
			printHeader = false;
			if (printThreads) {
				printf("\n%-37s %5s %8s %4s %8s %8s\n", "Thread", "Id", \
					"State", "Prio", "UTime", "KTime");
				printTeamThreads(&teamInfo,printSemaphoreInfo);
				printf("----------------------------------------------" \
					"-----------------------------\n");
				printHeader = true;
			}
		}
	} else {
		while (get_next_team_info(&teamCookie, &teamInfo) >= B_OK) {
			char *p;
			p = teamInfo.args;
			if ((p = strchr(p, ' ')))
				*p = '\0'; /* remove arguments, keep only argv[0] */
			p = strrchr(teamInfo.args, '/'); /* forget the path */
			if (p == NULL)
				p = teamInfo.args;
			if (strstr(p, string_to_match) == NULL)
				continue;
			printTeamInfo(&teamInfo,true);
			printf("\n%-37s %5s %8s %4s %8s %8s\n", "Thread", "Id", "State", \
				"Prio", "UTime", "KTime");
			printTeamThreads(&teamInfo,printSemaphoreInfo);
		}
	}

	if (printSystemInfo) {
		// system stats
		get_system_info(&systemInfo);
		printf("\nSystem Info\n");
		printf("%" B_PRIu32 "k (%" B_PRIu32 " bytes) total memory\n",
			(systemInfo.max_pages * B_PAGE_SIZE / 1024),
			(systemInfo.max_pages * B_PAGE_SIZE));
		printf("%" B_PRIu32 "k (%" B_PRIu32 " bytes) currently committed\n",
			(systemInfo.used_pages * B_PAGE_SIZE / 1024),
			(systemInfo.used_pages * B_PAGE_SIZE));
		printf("%" B_PRIu32 "k (%" B_PRIu32 " bytes) currently available\n",
			(systemInfo.max_pages - systemInfo.used_pages) * B_PAGE_SIZE / 1024,
			(systemInfo.max_pages - systemInfo.used_pages) * B_PAGE_SIZE);
		printf("%2.1f%% memory utilisation\n",
			(float)100 * systemInfo.used_pages / systemInfo.max_pages);
	}
	return 0;
}
