/* ps.c - process list
 * (c) 2002, François Revol (mmu_man) for OpenBeOS
 * released under the MIT licence.
 *
 * ChangeLog:
 * 04-26-2002 v1.0
 *  Initial.
 * 10-08-2002
 *  Added team name filtering (upon suggestion from Dano users :)
 * 
 */

#include <OS.h>
#include <stdio.h>
#include <string.h>

#define SNOOZE_TIME 100000

#define PS_HEADER " thread           name      state prio   user  kernel semaphore"
#define PS_SEP "-----------------------------------------------------------------------"

int main(int argc, char **argv)
{
	status_t ret;
	team_info teaminfo;
	thread_info thinfo;
	uint32 teamcookie = 0;
	int32 thcookie;
	sem_info sinfo;
	char *thstate;
	char *states[] = {"run", "rdy", "msg", "zzz", "sus", "sem" };
	system_info sysinfo;
	char *string_to_match; // match this in team name

	string_to_match = (argc == 2) ? argv[1] : NULL;
	puts("");
	puts(PS_HEADER);
	puts(PS_SEP);
	while ((ret = get_next_team_info(&teamcookie, &teaminfo)) >= B_OK) {
		if (string_to_match) {
			char *p;
			p = teaminfo.args;
			if ((p = strchr(p, ' ')))
				*p = '\0'; /* remove arguments, keep only argv[0] */
			p = strrchr(teaminfo.args, '/'); /* forget the path */
			if (p == NULL)
				p = teaminfo.args;
			if (strstr(p, string_to_match) == NULL)
				continue;
		}
		printf("%s (team %ld) (uid %d) (gid %d)\n", teaminfo.args, teaminfo.team, teaminfo.uid, teaminfo.gid);
		thcookie = 0;
		while ((ret = get_next_thread_info(teaminfo.team, &thcookie, &thinfo)) >= B_OK) {
			if (thinfo.state < B_THREAD_RUNNING || thinfo.state >B_THREAD_WAITING)
				thstate = "???";
			else
				thstate = states[thinfo.state-1];
			printf("%7ld %20s  %3s %3ld %7lli %7lli ", thinfo.thread, thinfo.name, thstate, thinfo.priority, thinfo.user_time/1000, thinfo.kernel_time/1000);
			if (thinfo.state == B_THREAD_WAITING) {
				get_sem_info(thinfo.sem, &sinfo);
				printf("%s(%ld)\n", sinfo.name, sinfo.sem);
			} else
				puts("");
		}
	}
	puts("");
	// system stats
	get_system_info(&sysinfo);
	printf("%lik (%li bytes) total memory\n", sysinfo.max_pages*B_PAGE_SIZE/1024, sysinfo.max_pages*B_PAGE_SIZE);
	printf("%lik (%li bytes) currently committed\n", sysinfo.used_pages*B_PAGE_SIZE/1024, sysinfo.used_pages*B_PAGE_SIZE);
	printf("%lik (%li bytes) currently available\n", (sysinfo.max_pages-sysinfo.used_pages)*B_PAGE_SIZE/1024, (sysinfo.max_pages-sysinfo.used_pages)*B_PAGE_SIZE);
	printf("%2.1f%% memory utilisation\n", (float)100 * sysinfo.used_pages / sysinfo.max_pages);
	return 0;
}

