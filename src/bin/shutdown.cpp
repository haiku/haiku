/*
 * Copyright 2002-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Francois Revol (mmu_man@users.sf.net)
 */


/*!	Shuts down the system, either halting or rebooting. */


#include <syscalls.h>

#include <OS.h>
#include <Roster.h>
#include <RosterPrivate.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>


uint32 gTimeToSleep = 0;
bool gReboot = false;


/*!	We get here when shutdown is cancelled.
	Then sleep() returns.
*/
void
handle_usr1(int sig)
{
	while (0);
}


bool
parseTime(char *arg, char *argv, int32 *_i)
{
	char *unit;

	if (isdigit(arg[0])) {
		gTimeToSleep = strtoul(arg, &unit, 10);
	} else if (argv && isdigit(argv[0])) {
		(*_i)++;
		gTimeToSleep = strtoul(argv, &unit, 10);
	} else
		return false;

	if (unit[0] == '\0' || strcmp(unit, "s") == 0)
		return true;
	if (strcmp(unit, "m") == 0) {
		gTimeToSleep *= 60;
		return true;
	}

	return false;
}


void
usage(const char *program)
{
	fprintf(stderr, "usage: %s [-rqca] [-d time]\n"
		"\t-r reboot,\n"
		"\t-q quick shutdown (don't broadcast apps),\n"
		"\t-a ask user to confirm the shutdown (ignored when -q is given),\n"
		"\t-c cancel a running shutdown,\n"
		"\t-s run shutdown synchronously (only returns if shutdown is cancelled)\n"
		"\t-d delay shutdown by <time> seconds.\n", program);
	exit(1);
}


int
main(int argc, char **argv)
{
	bool askUser = false;
	bool quick = false;
	bool async = true;

	const char *program = strrchr(argv[0], '/');
	if (program == NULL)
		program = argv[0];
	else
		program++;

	// handle 'halt' and 'reboot' symlinks
	if (strcmp(program, "reboot") == 0)
		gReboot = true;
	if (strcmp(program, "shutdown") != 0)
		askUser = true;

	for (int32 i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (arg[0] == '-') {
			if (!isalpha(arg[1]))
				usage(program);

			while (arg && isalpha((++arg)[0])) {
				switch (arg[0]) {
					case 'a':
						askUser = true;
						break;
					case 'q':
						quick = true;
						break;
					case 'r':
						gReboot = true;
						break;
					case 's':
						async = false;
						break;
					case 'c':
					{
						// find all running shutdown commands and signal their
						// shutdown threads

						thread_info threadInfo;
						get_thread_info(find_thread(NULL), &threadInfo);

						team_id thisTeam = threadInfo.team;

						int32 team_cookie = 0;
						team_info teamInfo;
						while (get_next_team_info(&team_cookie, &teamInfo)
								== B_OK) {
							if (strstr(teamInfo.args, "shutdown") != NULL
								&& teamInfo.team != thisTeam) {
								int32 thread_cookie = 0;
								while (get_next_thread_info(teamInfo.team,
										&thread_cookie, &threadInfo) == B_OK) {
									if (strcmp(threadInfo.name, "shutdown") == 0)
										kill(threadInfo.thread, SIGUSR1);
								}
							}
						}
						exit(0);
						break;
					}
					case 'd':
						if (parseTime(arg + 1, argv[i + 1], &i)) {
							arg = NULL;
							break;
						}
						// supposed to fall through

					default:
						usage(program);
				}
			}
		} else
			usage(program);
	}

	if (gTimeToSleep > 0) {
		int32 left;

		signal(SIGUSR1, handle_usr1);

		printf("Delaying %s by %" B_PRIu32 " seconds...\n",
			gReboot ? "reboot" : "shutdown", gTimeToSleep);

		left = sleep(gTimeToSleep);

		if (left > 0) {
			fprintf(stderr, "Shutdown cancelled.\n");
			exit(0);
		}
	}

	if (quick) {
		#ifdef __HAIKU__
		_kern_shutdown(gReboot);
		#endif // __HAIKU__
		fprintf(stderr, "Shutdown failed! (Do you have ACPI enabled?)\n");
		return 2;
	} else {
		BRoster roster;
		BRoster::Private rosterPrivate(roster);
		status_t error = rosterPrivate.ShutDown(gReboot, askUser, !async);
		if (error != B_OK) {
			fprintf(stderr, "Shutdown failed: %s\n", strerror(error));
			return 2;
		}
	}

	return 0;
}

