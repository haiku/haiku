/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "CliDumpMemoryCommand.h"

#include <ctype.h>
#include <stdio.h>

#include <AutoLocker.h>
#include <ExpressionParser.h>

#include "CliContext.h"
#include "Team.h"
#include "TeamMemoryBlock.h"
#include "UiUtils.h"
#include "UserInterface.h"


CliDumpMemoryCommand::CliDumpMemoryCommand()
	:
	CliCommand("dump contents of debugged team's memory",
		"%s [\"]address|expression[\"] [num]\n"
		"Reads and displays the contents of memory at the target address.")
{
}


void
CliDumpMemoryCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc < 2) {
		PrintUsage(argv[0]);
		return;
	}

	target_addr_t address;
	ExpressionParser parser;
	parser.SetSupportHexInput(true);

	try {
		address = parser.EvaluateToInt64(argv[1]);
	} catch(...) {
		printf("Error parsing address/expression.\n");
		return;
	}

	int32 itemSize = 0;
	int32 displayWidth = 0;

	// build the format string
	if (strcmp(argv[0], "db") == 0) {
		itemSize = 1;
		displayWidth = 16;
	} else if (strcmp(argv[0], "ds") == 0) {
		itemSize = 2;
		displayWidth = 8;
	} else if (strcmp(argv[0], "dw") == 0) {
		itemSize = 4;
		displayWidth = 4;
	} else if (strcmp(argv[0], "dl") == 0) {
		itemSize = 8;
		displayWidth = 2;
	} else if (strcmp(argv[0], "string") == 0) {
		itemSize = 1;
		displayWidth = -1;
	} else {
		printf("dump called in an invalid way!\n");
		return;
	}

	int32 num = 0;
	if (argc == 3) {
		char *remainder;
		num = strtol(argv[2], &remainder, 0);
		if (*remainder != '\0') {
			printf("Error: invalid parameter \"%s\"\n", argv[2]);
		}
	}

	if (num <= 0)
		num = displayWidth;

	TeamMemoryBlock* block = context.CurrentBlock();
	if (block == NULL || !block->Contains(address)) {
		context.GetUserInterfaceListener()->InspectRequested(address,
			&context);
		context.WaitForEvents(CliContext::EVENT_TEAM_MEMORY_BLOCK_RETRIEVED);
		if (context.IsTerminating())
			return;
		block = context.CurrentBlock();
	}

	if (!strcmp(argv[0], "string")) {
		printf("%p \"", (char*)address);

		target_addr_t offset = address;
		char c;
		while (block->Contains(offset)) {
			c = *(block->Data() + offset - block->BaseAddress());

			if (c == '\0')
				break;
			if (c == '\n')
				printf("\\n");
			else if (c == '\t')
				printf("\\t");
			else {
				if (!isprint(c))
					c = '.';

				printf("%c", c);
			}
			++offset;
		}

		printf("\"\n");
	} else {
		BString output;
		UiUtils::DumpMemory(output, 0, block, address, itemSize, displayWidth,
			num);
		printf("%s\n", output.String());
	}
}
