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
	free(malloc(10));
	sHandledSignals++;
}


int
main()
{
	const int testSeconds = 2;
	printf("Trying to provoke allocator deadlock in signal handler.\n");
	printf("If successful test will finish in %d seconds...\n", testSeconds);

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

	bigtime_t endTime = system_time() + 1000000 * (bigtime_t)testSeconds;
	while (system_time() < endTime) {
		const int allocationCount = 1000;
		void* allocations[allocationCount];
		for (int i = 0; i < allocationCount; i++)
			allocations[i] = malloc(rand() % 50);
		for (int i = 0; i < allocationCount; i++)
			free(allocations[i]);
	}

	kill_thread(signalThread);
	snooze(1000);

	printf("test successful, handled %lld signals\n", sHandledSignals);

	return 0;
}
