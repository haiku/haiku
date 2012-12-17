/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_STACK_FRAME_COMMAND_H
#define CLI_STACK_FRAME_COMMAND_H


#include "CliCommand.h"


class CliStackFrameCommand : public CliCommand {
public:
								CliStackFrameCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_STACK_FRAME_COMMAND_H
