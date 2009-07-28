/* waitfor.c - waits for a given threadname
 * (c) 2002, François Revol (mmu_man) for OpenBeOS
 * released under the MIT licence.
 *
 * ChangeLog:
 * 04-26-2002 v1.0
 *  Initial.
 * 
 * waitfor threadname
 * thesnooze() time is the same as the original, found using bdb waitfor foobar,
 * and stepping until the snooze() call returns, the value is at the push 
 * instruction just before the call.
 */

#include <OS.h>
#include <stdio.h>

#define SNOOZE_TIME 100000

int main(int argc, char **argv)
{
	status_t ret;

	if (argc == 2) {
		while (find_thread(argv[1]) < 0) {
			if ((ret = snooze(SNOOZE_TIME)) < B_OK)
				return 1;
		}
	} else if (argc == 3 && strcmp(argv[1], "-e") == 0) {
		while (find_thread(argv[2]) >= 0) {
			if ((ret = snooze(SNOOZE_TIME)) < B_OK)
				return 1;
		}
	} else {
		fprintf(stderr, "Usage: %s [-e] thread_name\n"
			"-e : wait until all threads with that name ended\n", argv[0]);
		return 1;
	}

	return 0;
}

