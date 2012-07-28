/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliThreadsCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "Team.h"
#include "UiUtils.h"


CliThreadsCommand::CliThreadsCommand()
	:
	CliCommand("list the team's threads",
		"%s\n"
		"Lists the team's threads.")
{
}


void
CliThreadsCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	Team* team = context.GetTeam();
	AutoLocker<Team> teamLocker(team);

	printf("        ID  state      name\n");
	printf("----------------------------\n");

	for (ThreadList::ConstIterator it = team->Threads().GetIterator();
		 	Thread* thread = it.Next();) {
		const char* stateString = UiUtils::ThreadStateToString(
			thread->State(), thread->StoppedReason());
		printf("%10" B_PRId32 "  %-9s  \"%s\"\n", thread->ID(), stateString,
			thread->Name());
	}
}
