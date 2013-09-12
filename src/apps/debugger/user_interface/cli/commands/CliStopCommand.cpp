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
	CliCommand("stop the current thread",
		"%s\n"
		"Stops the current thread.")
{
}


void
CliStopCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	AutoLocker<Team> teamLocker(context.GetTeam());
	Thread* thread = context.CurrentThread();
	if (thread == NULL) {
		printf("Error: No current thread.\n");
		return;
	}

	if (thread->State() == THREAD_STATE_STOPPED) {
		printf("Error: The current thread is already stopped.\n");
		return;
	}

	context.GetUserInterfaceListener()->ThreadActionRequested(thread->ID(),
		MSG_THREAD_STOP);
}
