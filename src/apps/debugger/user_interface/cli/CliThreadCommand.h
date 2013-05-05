/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_THREAD_COMMAND_H
#define CLI_THREAD_COMMAND_H


#include "CliCommand.h"


class CliThreadCommand : public CliCommand {
public:
								CliThreadCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_THREAD_COMMAND_H
