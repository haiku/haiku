/* renice.c - unixish renice command for BeOs
 * (c) 2001, 2002, François Revol (mmu_man) for Haiku
 * released under the MIT licence.
 *
 * How did I live without it before ??? ;)
 * ChangeLog:
 * 04-25-2002 v1.2
 *  Cleanup for inclusion in Haiku,
 *  Used the code to rewrite the 'prio' BeOS command for Haiku.
 * 04-14-2002 v1.1
 *  Added -f upon suggestion from Idéfix on BeShare
 * 2001 v1.0
 *  Initial.
 */

#include <OS.h>
#include <stdio.h>
#include <string.h>

/*
   Notes:

From man renice:
/usr/sbin/renice [-n increment] [-p] [-g | -u] ID ... /usr/sbin/renice priority [-p] pid ... [-g pgrp ...] [-u user ...]


BeOs priorities:
(realtime)	High Prio			Default					Low Prio
120			99					10						1 (0 only for idle_thread)

UNIX nice:
			-20					0						19
UNIX priorities:
			0					20						39

renice can be given more than one pid on the command line.
prio is locked into one pid, then the priority.

*/

#ifndef NZERO
#define NZERO 20
#endif

#define BZERO B_NORMAL_PRIORITY
#define BMIN (B_REAL_TIME_DISPLAY_PRIORITY-1)
//#define BMAX B_NORMAL_PRIORITY
#define BMAX 1

// returns an equivalent UNIX priority for a given BeOS priority.
static int32 prio_be_to_unix(int32 prio)
{
	if (prio > BZERO)
		return NZERO - ((prio - BZERO) * NZERO) / (BMIN - BZERO);
	return NZERO + ((BZERO - prio) * (NZERO - 1)) / (BZERO - BMAX);
}

// returns an equivalent BeOS priority for a given UNIX priority.
static int32 prio_unix_to_be(int32 prio)
{
	if (prio > NZERO)
		return BZERO - ((prio - NZERO) * (BZERO - BMAX)) / (NZERO-1);
	return BZERO + ((NZERO - prio) * (BMIN - BZERO)) / (NZERO);
}

static status_t renice_thread(int32 prio, int32 increment, bool use_be_prio, thread_id th)
{
	thread_info thinfo;

	if(increment != 0) {
		get_thread_info(th, &thinfo);
		prio = thinfo.priority;
		if(!use_be_prio)
			prio = prio_be_to_unix(prio);
		prio += increment;
		if(!use_be_prio)
			prio = prio_unix_to_be(prio);
	}
	return set_thread_priority(th, prio);
}

int main(int argc, char **argv)
{
	thread_id th = -1;
	int32 prio, increment = 0;
	thread_info thinfo;
	bool use_be_prio = false;
	bool next_is_prio = true;
	bool next_is_increment = false;
	bool use_increment = false;
	bool find_by_name = false;
	int i = 0;
	int32 teamcookie = 0;
	team_info teaminfo;
	int32 thcookie = 0;
	int err = 1;
	char *thname;

	prio = NZERO; // default UNIX priority for nice
	// convert it to beos
	if (!use_be_prio)
	prio = prio_unix_to_be(prio);
	
	while (++i < argc) {
		if (!strcmp(argv[i], "-p")) { // ignored option
		} else if (!strcmp(argv[i], "-n")) {
			next_is_prio = false;
			next_is_increment = true;
			use_increment = true;
		} else if (!strcmp(argv[i], "-b")) {
			use_be_prio = true;
		} else if (!strcmp(argv[i], "-f")) {
			find_by_name = true;
		} else if (next_is_increment) {
			next_is_increment = false;
			sscanf(argv[i], "%ld", (long *)&increment);
		} else if (next_is_prio) {
			next_is_prio = false;
			sscanf(argv[i], "%ld", (long *)&prio);
			if (!use_be_prio)
				prio = prio_unix_to_be(prio);
		} else {
			if (!find_by_name) {
			sscanf(argv[i], "%ld", (long *)&th);
				return (renice_thread(prio, increment, use_be_prio, th) == B_OK)?0:1;
			}
			thname = argv[i];
			while (get_next_team_info(&teamcookie, &teaminfo) == B_OK) {
				thcookie = 0;
				while (get_next_thread_info(teaminfo.team, &thcookie, &thinfo) == B_OK) {
					if (!strncmp(thname, thinfo.name, B_OS_NAME_LENGTH)) {
						th = thinfo.thread;
						renice_thread(prio, increment, use_be_prio, th);
						err = 0;
						/* find another one */
					}
				}
			}
			return err;
		}
	}
	if (th == -1) {
		puts("Usage:");
		puts("renice [-b] [-n increment | prio] [-f thname thname...|thid thid ...]");
		puts("	-b : use BeOS priorities instead of UNIX priorities");
		puts("	-n : adds increment to the current priority instead of assigning it");
		puts("	-f : find threads by name instead of by id");
		return 1;
	}
	return 0;
}

