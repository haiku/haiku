/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliQuitCommand.h"

#include <String.h>

#include "CliContext.h"


CliQuitCommand::CliQuitCommand()
	:
	CliCommand("quit Debugger",
		"%s\n"
		"Quits Debugger.")
{
}


void
CliQuitCommand::Execute(int argc, const char* const* argv, CliContext& context)
{
	// Ask the user what to do with the debugged team.
	printf("Kill or resume the debugged team?\n");
	for (;;) {
		const char* line = context.PromptUser("(k)ill, (r)esume, (c)ancel? ");
		if (line == NULL)
			return;

		BString trimmedLine(line);
		trimmedLine.Trim();

		if (trimmedLine == "k") {
			context.QuitSession(true);
			break;
		}

		if (trimmedLine == "r") {
			context.QuitSession(false);
			break;
		}

		if (trimmedLine == "c")
			break;
	}
}
