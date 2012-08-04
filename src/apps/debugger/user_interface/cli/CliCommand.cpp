/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliCommand.h"

#include <stdio.h>


CliCommand::CliCommand(const char* summary, const char* usage)
	:
	fSummary(summary),
	fUsage(usage)
{
}


CliCommand::~CliCommand()
{
}


void
CliCommand::PrintUsage(const char* commandName) const
{
	printf("Usage: ");
	printf(Usage(), commandName);
	printf("\n");
}
