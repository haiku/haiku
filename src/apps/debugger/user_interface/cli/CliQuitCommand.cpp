/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliQuitCommand.h"

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
	context.QuitSession();
}
