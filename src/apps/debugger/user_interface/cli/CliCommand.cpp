/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliCommand.h"


CliCommand::CliCommand(const char* summary, const char* usage)
	:
	fSummary(summary),
	fUsage(usage)
{
}


CliCommand::~CliCommand()
{
}
