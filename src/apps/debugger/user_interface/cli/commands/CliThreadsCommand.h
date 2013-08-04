/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_THREADS_COMMAND_H
#define CLI_THREADS_COMMAND_H


#include "CliCommand.h"


class CliThreadsCommand : public CliCommand {
public:
								CliThreadsCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_THREADS_COMMAND_H
