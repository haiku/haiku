/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_STACK_TRACE_COMMAND_H
#define CLI_STACK_TRACE_COMMAND_H


#include "CliCommand.h"


class CliStackTraceCommand : public CliCommand {
public:
								CliStackTraceCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_STACK_TRACE_COMMAND_H
