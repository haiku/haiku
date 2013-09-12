/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_QUIT_COMMAND_H
#define CLI_QUIT_COMMAND_H


#include "CliCommand.h"


class CliQuitCommand : public CliCommand {
public:
								CliQuitCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_QUIT_COMMAND_H
