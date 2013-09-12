/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_DEBUG_REPORT_COMMAND_H
#define CLI_DEBUG_REPORT_COMMAND_H


#include "CliCommand.h"


class CliDebugReportCommand : public CliCommand {
public:
								CliDebugReportCommand();
	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);
};


#endif	// CLI_DEBUG_REPORT_COMMAND_H
