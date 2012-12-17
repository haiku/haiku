/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CliStackFrameCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "FunctionInstance.h"
#include "StackFrame.h"
#include "StackTrace.h"
#include "Team.h"


CliStackFrameCommand::CliStackFrameCommand()
	:
	CliCommand("set current stack frame",
		"%s [ <frame number> ]\n"
		"Sets the current stack frame to <frame number>, if supplied. "
		"Otherwise\n prints the current frame.")
{
}


void
CliStackFrameCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	if (argc > 2) {
		PrintUsage(argv[0]);
		return;
	}

	StackTrace* stackTrace = context.GetStackTrace();
	if (argc == 1) {
		int32 currentFrameIndex = context.CurrentStackFrameIndex();
		if (currentFrameIndex < 0)
			printf("No current frame.\n");
		else {
			StackFrame* frame = stackTrace->FrameAt(currentFrameIndex);
			printf("Current frame: %" B_PRId32 ": %s\n", currentFrameIndex,
				frame->Function()->PrettyName().String());
		}
		return;
	}
	// parse the argument
	char* endPointer;
	int32 frameNumber = strtol(argv[1], &endPointer, 0);
	if (*endPointer != '\0' || frameNumber < 0) {
		printf("Error: Invalid parameter \"%s\"\n", argv[1]);
		return;
	}

	if (stackTrace == NULL || frameNumber >= stackTrace->CountFrames()) {
		printf("Error: Index %" B_PRId32 " out of range\n", frameNumber);
		return;
	} else
		context.SetCurrentStackFrameIndex(frameNumber);
}
