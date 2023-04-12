/*
 * Copyright 2012-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_DUMP_STRING_COMMAND_H
#define CLI_DUMP_STRING_COMMAND_H


#include "CliCommand.h"


class SourceLanguage;


class CliDumpStringCommand : public CliCommand {
public:
								CliDumpStringCommand();
	virtual						~CliDumpStringCommand();

	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);

private:
	SourceLanguage*				fLanguage;
};


#endif	// CLI_DUMP_STRING_COMMAND_H
