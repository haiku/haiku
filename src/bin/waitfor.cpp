/* waitfor.c - waits for a given threadname
 * (c) 2002, François Revol (mmu_man) for Haiku
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

#include <stdio.h>
#include <string.h>

#include <Messenger.h>


#define SNOOZE_TIME 100000


int
main(int argc, char** argv)
{
	if (argc == 2) {
		while (find_thread(argv[1]) < 0) {
			if (snooze(SNOOZE_TIME) != B_OK)
				return 1;
		}
	} else if (argc == 3 && strcmp(argv[1], "-e") == 0) {
		while (find_thread(argv[2]) >= 0) {
			if (snooze(SNOOZE_TIME) != B_OK)
				return 1;
		}
	} else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
		while (true) {
			BMessenger messenger(argv[2]);
			if (messenger.IsValid())
				break;
			if (snooze(SNOOZE_TIME) != B_OK)
				return 1;
		}
	} else {
		fprintf(stderr,
			"Usage:\n"
			"  %s <thread_name>\n"
			"      wait until a thread with 'thread_name' has been started.\n\n"
			"  %s -e <thread_name>\n"
			"      wait until all threads with thread_name have ended.\n\n"
			"  %s -m <app_signature>\n"
			"      wait until the application specified by 'app_signature' is "
			"is ready to receive messages.\n", argv[0], argv[0], argv[0]);
		return 1;
	}

	return 0;
}

