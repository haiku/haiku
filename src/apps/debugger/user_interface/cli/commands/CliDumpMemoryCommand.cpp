/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "CliDumpMemoryCommand.h"

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


CliDumpMemoryCommand::CliDumpMemoryCommand()
	:
	CliCommand("dump contents of debugged team's memory",
		"%s [\"]address|expression[\"] [num]\n"
		"Reads and displays the contents of memory at the target address.")
{
	fLanguage = new(std::nothrow) CppLanguage();
}


CliDumpMemoryCommand::~CliDumpMemoryCommand()
{
	if (fLanguage != NULL)
		fLanguage->ReleaseReference();
}


void
CliDumpMemoryCommand::Execute(int argc, const char* const* argv,
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

	ExpressionInfo* info = context.GetExpressionInfo();

	target_addr_t address = 0;
	info->SetTo(argv[1]);

	context.GetUserInterfaceListener()->ExpressionEvaluationRequested(
		fLanguage, info);
	context.WaitForEvents(CliContext::EVENT_EXPRESSION_EVALUATED);
	if (context.IsTerminating())
		return;

	BString errorMessage;
	ExpressionResult* result = context.GetExpressionValue();
	if (result != NULL) {
		if (result->Kind() == EXPRESSION_RESULT_KIND_PRIMITIVE) {
			Value* value = result->PrimitiveValue();
			BVariant variantValue;
			value->ToVariant(variantValue);
			if (variantValue.Type() == B_STRING_TYPE)
				errorMessage.SetTo(variantValue.ToString());
			else
				address = variantValue.ToUInt64();
		}
	} else
		errorMessage = strerror(context.GetExpressionResult());

	if (!errorMessage.IsEmpty()) {
		printf("Unable to evaluate expression: %s\n",
			errorMessage.String());
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
