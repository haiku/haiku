/*
 * Copyright 2012-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CliPrintVariableCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "StackFrame.h"
#include "StackTrace.h"
#include "Team.h"
#include "Type.h"
#include "UiUtils.h"
#include "UserInterface.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"
#include "ValueNodeManager.h"


CliPrintVariableCommand::CliPrintVariableCommand()
	:
	CliCommand("print value(s) of a variable",
		"%s [--depth n] variable [variable2 ...]\n"
		"Prints the value and members of the named variable.")
{
}


void
CliPrintVariableCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc < 2) {
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

	int32 depth = 1;
	int32 i = 1;
	for (; i < argc; i++) {
		if (strcmp(argv[i], "--depth") == 0) {
			if (i == argc - 1) {
				printf("Error: An argument must be supplied for depth.\n");
				return;
			}
			char* endPointer;
			depth = strtol(argv[i + 1], &endPointer, 0);
			if (*endPointer != '\0' || depth < 0) {
				printf("Error: Invalid parameter \"%s\"\n", argv[i + 1]);
				return;
			}
			i++;
		}
		else
			break;
	}

	if (i == argc) {
		printf("Error: At least one variable name must be supplied.\n");
		return;
	}

	bool found = false;
	while (i < argc) {
		// TODO: support variable expressions in addition to just names.
		const char* variableName = argv[i++];
		for (int32 j = 0; ValueNodeChild* child = container->ChildAt(j); j++) {
			if (child->Name() == variableName) {
				found = true;
				containerLocker.Unlock();
				_ResolveValueIfNeeded(child->Node(), context, depth);
				containerLocker.Lock();
				BString data;
				UiUtils::PrintValueNodeGraph(data, child, 1, depth);
				printf("%s", data.String());
			}
		}

		if (!found)
			printf("No such variable: %s\n", variableName);
		found = false;
	}
}


status_t
CliPrintVariableCommand::_ResolveValueIfNeeded(ValueNode* node,
	CliContext& context, int32 maxDepth)
{
	StackFrame* frame = context.GetStackTrace()->FrameAt(
		context.CurrentStackFrameIndex());
	if (frame == NULL)
		return B_BAD_DATA;

	status_t result = B_OK;
	ValueNodeManager* manager = context.GetValueNodeManager();
	ValueNodeContainer* container = manager->GetContainer();
	AutoLocker<ValueNodeContainer> containerLocker(container);
	if (node->LocationAndValueResolutionState() == VALUE_NODE_UNRESOLVED) {
		context.GetUserInterfaceListener()->ValueNodeValueRequested(
			context.CurrentThread()->GetCpuState(), container, node);


		while (node->LocationAndValueResolutionState()
			== VALUE_NODE_UNRESOLVED) {
			containerLocker.Unlock();
			context.WaitForEvent(CliContext::MSG_VALUE_NODE_CHANGED);
			containerLocker.Lock();
			if (context.IsTerminating())
				return B_ERROR;
		}
	}

	if (node->LocationAndValueResolutionState() == B_OK && maxDepth > 0) {
		for (int32 i = 0; i < node->CountChildren(); i++) {
			ValueNodeChild* child = node->ChildAt(i);
			containerLocker.Unlock();
			result = manager->AddChildNodes(child);
			if (result != B_OK)
				continue;

			// since in the case of a pointer to a compound we hide
			// the intervening compound, don't consider the hidden node
			// a level for the purposes of depth traversal
			if (node->GetType()->Kind() == TYPE_ADDRESS
				&& child->GetType()->Kind() == TYPE_COMPOUND) {
				_ResolveValueIfNeeded(child->Node(), context, maxDepth);
			} else
				_ResolveValueIfNeeded(child->Node(), context, maxDepth - 1);
			containerLocker.Lock();
		}
	}

	return result;
}
