/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_CONTINUE_COMMAND_H
#define CLI_CONTINUE_COMMAND_H


#include "CliCommand.h"


class CliContinueCommand : public CliCommand {
public:
								CliContinueCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_CONTINUE_COMMAND_H
