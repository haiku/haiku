/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_STOP_COMMAND_H
#define CLI_STOP_COMMAND_H


#include "CliCommand.h"


class CliStopCommand : public CliCommand {
public:
								CliStopCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_STOP_COMMAND_H
