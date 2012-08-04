/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliThreadCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "Team.h"


CliThreadCommand::CliThreadCommand()
	:
	CliCommand("set or print the current thread",
		"%s [ <thread ID> ]\n"
		"Sets the current thread to <thread ID>, if supplied. Otherwise prints "
			"the\n"
		"current thread.")
{
}


void
CliThreadCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc > 2) {
		PrintUsage(argv[0]);
		return;
	}

	if (argc < 2) {
		// no arguments -- print the current thread
		context.PrintCurrentThread();
		return;
	}

	// parse the argument
	char* endPointer;
	long threadID = strtol(argv[1], &endPointer, 0);
	if (*endPointer != '\0' || threadID < 0) {
		printf("Error: Invalid parameter \"%s\"\n", argv[1]);
		return;
	}

	// get the thread and change the current thread
	Team* team = context.GetTeam();
	AutoLocker<Team> teamLocker(team);
	if (Thread* thread = team->ThreadByID(threadID))
		context.SetCurrentThread(thread);
	else
		printf("Error: No thread with ID %ld\n", threadID);
}
