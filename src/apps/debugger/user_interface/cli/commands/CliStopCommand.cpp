/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliStopCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "MessageCodes.h"
#include "Team.h"
#include "UserInterface.h"

CliStopCommand::CliStopCommand()
	:
	CliCommand("stop a thread",
		"%s [ <thread ID> ]\n"
		"Stops the thread specified by <thread ID>, if supplied. Otherwise "
			"stops\n"
		"the current thread.")
{
}


void
CliStopCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc > 2) {
		PrintUsage(argv[0]);
		return;
	}


	AutoLocker<Team> teamLocker(context.GetTeam());
	Thread* thread = NULL;
	if (argc < 2) {
		thread = context.CurrentThread();
		if (thread == NULL) {
			printf("Error: No current thread.\n");
			return;
		}
	} else if (argc == 2) {
		// parse the argument
		char* endPointer;
		long threadID = strtol(argv[1], &endPointer, 0);
		if (*endPointer != '\0' || threadID < 0) {
			printf("Error: Invalid parameter \"%s\"\n", argv[1]);
			return;
		}

		// get the thread and change the current thread
		Team* team = context.GetTeam();
		thread = team->ThreadByID(threadID);
		if (thread == NULL) {
			printf("Error: No thread with ID %ld\n", threadID);
			return;
		}
	}

	if (thread->State() == THREAD_STATE_STOPPED) {
		printf("Error: thread %" B_PRId32 " is already stopped.\n",
			thread->ID());
		return;
	}

	context.GetUserInterfaceListener()->ThreadActionRequested(thread->ID(),
		MSG_THREAD_STOP);
}
