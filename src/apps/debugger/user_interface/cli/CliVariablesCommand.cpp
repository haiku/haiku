/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CliVariablesCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "Team.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"
#include "ValueNodeManager.h"


CliVariablesCommand::CliVariablesCommand()
	:
	CliCommand("show current frame variables",
		"%s\n"
		"Prints the parameters and variables of the current frame, if "
			" available.")
{
}


void
CliVariablesCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc > 1) {
		PrintUsage(argv[0]);
		return;
	}

	ValueNodeManager* manager = context.GetValueNodeManager();

	ValueNodeContainer* container = manager->GetContainer();
	AutoLocker<ValueNodeContainer> containerLocker(container);
	if (container == NULL || container->CountChildren() == 0) {
		printf("No variables available.\n");
		return;
	}

	printf("Variables:\n");
	for (int32 i = 0; ValueNodeChild* child = container->ChildAt(i); i++) {
		printf("  %s\n", child->Name().String());
	}
}
