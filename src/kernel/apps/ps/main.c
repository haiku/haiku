/*
 * PS command
 * Rewritten for OpenBeOS by Angelo Mottola, Aug 2002.
 */

#include <OS.h>
#include <Errors.h>
#include <syscalls.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	int32 thread_num;
	int32 team_num = 0;
	thread_info thread;
	team_info team;
	
	printf("\n thread           name      state prio   user  kernel semaphore\n");
	printf("-----------------------------------------------------------------------\n");
	
	while (get_next_team_info(&team_num, &team) == B_OK) {
		printf("%s (team %d) (uid %d) (gid %d)\n",
			team.args, team.team, team.uid, team.gid);
		thread_num = 0;
		while (get_next_thread_info(team.team, &thread_num, &thread) == B_OK) {
			// Fix thread state and sem output once states are BeOS compatible
			printf(" %6d %20s  %s %3d %7d %7d %s\n",
				thread.thread, thread.name, "???", thread.priority,
				(int)thread.user_time, (int)thread.kernel_time, "");
		}
	}
	printf("\n");
	
	return 0;
}
