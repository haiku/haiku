/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <OS.h>


static int64 sHandledSignals = 0;


static status_t
signal_pusher(void* data)
{
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	thread_id mainThread = teamInfo.team;

	while (true) {
		send_signal(mainThread, SIGUSR1);
		snooze(1000);
	}

	return B_OK;
}


static void
signal_handler(int signal)
{
	sHandledSignals++;
}


static void
allocator_thread(int level)
{
	if (level > 100000)
		return;

	free(malloc(rand() % 10000));

	allocator_thread(level + 1);
}


int
main()
{
	// Test program to reproduce bug #2562. Is finished quickly and must be run
	// in a loop to reproduce the bug.

	// install signal handler
	if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
		fprintf(stderr, "Error: Failed to install signal handler: %s\n",
			strerror(errno));
		exit(1);
	}

	// start signal thread
	thread_id signalThread = spawn_thread(&signal_pusher, "signal pusher",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(signalThread);

	allocator_thread(0);

	kill_thread(signalThread);
	snooze(1000);

	printf("test successful, handled %lld signals\n", sHandledSignals);

	return 0;
}
