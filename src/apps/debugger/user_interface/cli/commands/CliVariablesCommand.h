/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_VARIABLES_COMMAND_H
#define CLI_VARIABLES_COMMAND_H


#include "CliCommand.h"


class CliVariablesCommand : public CliCommand {
public:
								CliVariablesCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_VARIABLES_COMMAND_H
