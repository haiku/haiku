/* renice.c - unixish renice command for BeOs
 * (c) 2001, 2002, François Revol (mmu_man) for OpenBeOS
 * released under the MIT licence.
 *
 * How did I live without it before ??? ;)
 * ChangeLog:
 * 04-25-2002 v1.2
 *  Cleanup for inclusion in OpenBeOS,
 *  Used the code to rewrite the 'prio' BeOS command for OpenBeOS.
 * 04-14-2002 v1.1
 *  Added -f upon suggestion from Idéfix on BeShare
 * 2001 v1.0
 *  Initial.
 */

#include <OS.h>
#include <stdio.h>

/*
   Notes:

From man renice:
/usr/sbin/renice [-n increment] [-p] [-g | -u] ID ... /usr/sbin/renice priority [-p] pid ... [-g pgrp ...] [-u user ...]


#define B_LOW_PRIORITY                  5
#define B_NORMAL_PRIORITY               10
#define B_DISPLAY_PRIORITY              15
#define B_URGENT_DISPLAY_PRIORITY       20
#define B_REAL_TIME_DISPLAY_PRIORITY    100
#define B_URGENT_PRIORITY               110
#define B_REAL_TIME_PRIORITY            120

BeOs priorities:
High Prio (realtime)            Default                  Low Prio
120                               10                        1 (0 only for idle_thread)

UNIX Priorities:
-20                               0                         20

Note that however this isn't perfect, since priorities 
beyond 100 in BeOS move the thread into the real-time class.
Linux for example, has a separate API for doing this, 
and even a process set to -20 can be in the normal scheduling class.

renice can be given more than one pid on the command line.
prio is locked into one pid, then the priority.

*/

// returns an equivalent UNIX priority for a given BeOS priority.
int32 prio_be_to_unix(int32 prio)
{
	return (prio > 10)?(- (20 * (prio - 10)) / 110):(2 * (10 - prio));
}

// returns an equivalent BeOS priority for a given UNIX priority.
int32 prio_unix_to_be(int32 prio)
{
	return (prio > 0)?(10 - (prio/2)):(10 + 110 * (-prio) / 20);
}

int main(int argc, char **argv)
{
	thread_id th = -1;
	int32 prio, increment;
	thread_info thinfo;
	bool use_be_prio = false;
	bool next_is_prio = true;
	bool next_is_increment = false;
	bool use_increment = false;
	bool find_by_name = false;
	int i = 0;

	prio = 9; // default UNIX priority for nice
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
			if (find_by_name)
				th = find_thread(argv[i]);
			else
				sscanf(argv[i], "%ld", (long *)&th);
			if (use_increment) {
				get_thread_info(th, &thinfo);
				prio = thinfo.priority;
				if (!use_be_prio)
					prio = prio_be_to_unix(prio);
				prio += increment;
				if (!use_be_prio)
					prio = prio_unix_to_be(prio);
			}
			set_thread_priority(th, prio);
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

