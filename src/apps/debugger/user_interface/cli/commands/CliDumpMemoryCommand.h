/*
 * Copyright 2012-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_DUMP_MEMORY_COMMAND_H
#define CLI_DUMP_MEMORY_COMMAND_H


#include "CliCommand.h"

#include <String.h>


class SourceLanguage;


class CliDumpMemoryCommand : public CliCommand {
public:
								CliDumpMemoryCommand(int itemSize,
										const char* itemSizeNoun,
										int displayWidth);
	virtual						~CliDumpMemoryCommand();

	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);

private:
	SourceLanguage*				fLanguage;
	BString						fSummaryString;
	BString						fUsageString;
	int							itemSize;
	int							displayWidth;
};


#endif	// CLI_DUMP_MEMORY_COMMAND_H
