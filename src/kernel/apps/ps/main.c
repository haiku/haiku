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


const char *state(thread_state state);


const char *state(thread_state state)
{
	switch (state) {
		case B_THREAD_RUNNING:
			return "run";
		case B_THREAD_READY:
			return "rdy";
		case B_THREAD_SUSPENDED:
			return "sus";
		case B_THREAD_WAITING:
			return "sem";
		case B_THREAD_ASLEEP:
			return "zzz";
		case B_THREAD_RECEIVING:
			return "msg";
		default:
			return "???";
	}
}


int main(int argc, char **argv)
{
	int32 thread_num;
	int32 team_num = 0;
	thread_info thread;
	team_info team;
	sem_info sem;
	char sem_name[B_OS_NAME_LENGTH * 2];
	char buffer[B_OS_NAME_LENGTH];
	
	printf("\n thread           name      state prio   user  kernel semaphore\n");
	printf("-----------------------------------------------------------------------\n");
	
	while (get_next_team_info(&team_num, &team) == B_OK) {
		printf("%s (team %ld) (uid %d) (gid %d)\n",
			team.args, team.team, team.uid, team.gid);
		thread_num = 0;
		while (get_next_thread_info(team.team, &thread_num, &thread) == B_OK) {
			sem_name[0] = '\0';
			if (thread.state == B_THREAD_WAITING) {
				if (get_sem_info(thread.sem, &sem) == B_OK) {
					strcpy(sem_name, sem.name);
					sprintf(buffer, "(%ld)", sem.sem);
					strcat(sem_name, buffer);
				}
			}
			printf(" %6ld %20s  %s %3ld %7d %7d %s\n",
				thread.thread, thread.name, state(thread.state), thread.priority,
				(int)thread.user_time, (int)thread.kernel_time, sem_name);
		}
	}
	printf("\n");
	
	return 0;
}
