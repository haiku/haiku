/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_WRITE_CORE_FILE_COMMAND_H
#define CLI_WRITE_CORE_FILE_COMMAND_H


#include "CliCommand.h"


class CliWriteCoreFileCommand : public CliCommand {
public:
								CliWriteCoreFileCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_WRITE_CORE_FILE_COMMAND_H
