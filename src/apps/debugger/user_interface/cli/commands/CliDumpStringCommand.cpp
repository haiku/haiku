/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "CliDumpStringCommand.h"

#include <ctype.h>
#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "CppLanguage.h"
#include "Team.h"
#include "TeamMemoryBlock.h"
#include "UiUtils.h"
#include "UserInterface.h"
#include "Value.h"
#include "Variable.h"


CliDumpStringCommand::CliDumpStringCommand()
	:
	CliCommand("dump contents of a string in the debugged team's memory",
			"%s [\"]address|expression[\"]\n"
			"Reads and displays the contents of a null-terminated string at the target address.")
{
	// TODO: this should be retrieved via some indirect helper rather
	// than instantiating the specific language directly.
	fLanguage = new(std::nothrow) CppLanguage();
}


CliDumpStringCommand::~CliDumpStringCommand()
{
	if (fLanguage != NULL)
		fLanguage->ReleaseReference();
}


void
CliDumpStringCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc < 2) {
		PrintUsage(argv[0]);
		return;
	}

	if (fLanguage == NULL) {
		printf("Unable to evaluate expression: %s\n", strerror(B_NO_MEMORY));
		return;
	}

	target_addr_t address;
	if (context.EvaluateExpression(argv[1], fLanguage, address) != B_OK)
		return;

	TeamMemoryBlock* block = NULL;
	if (context.GetMemoryBlock(address, block) != B_OK)
		return;

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
}
