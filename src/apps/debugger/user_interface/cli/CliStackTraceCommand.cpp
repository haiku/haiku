/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliStackTraceCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "FunctionInstance.h"
#include "StackTrace.h"
#include "Team.h"


CliStackTraceCommand::CliStackTraceCommand()
	:
	CliCommand("print a stack trace of the current thread",
		"%s\n"
		"Prints a stack trace for the current thread.")
{
}


void
CliStackTraceCommand::Execute(int argc, const char* const* argv,
	CliContext& context)
{
	// get the current thread
	Team* team = context.GetTeam();
	AutoLocker<Team> teamLocker(team);
	Thread* thread = context.CurrentThread();
	if (thread == NULL) {
		printf("no current thread\n");
		return;
	}

	if (thread->State() != THREAD_STATE_STOPPED) {
		printf("Current thread is not stopped. Can't get stack trace.\n");
		return;
	}

	// get its stack trace
	StackTrace* stackTrace = thread->GetStackTrace();
	if (stackTrace == NULL) {
		// TODO: Wait for stack trace!
		printf("Current thread doesn't have a stack trace. Waiting not "
			"implemented yet\n");
		return;
	}
	BReference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until we're done

	teamLocker.Unlock();

	// print the stack trace
	int32 frameCount = stackTrace->CountFrames();
	for (int32 i = 0; i < frameCount; i++) {
		StackFrame* frame = stackTrace->FrameAt(i);
		printf("%3" B_PRId32 "  %#" B_PRIx64 "  %#" B_PRIx64, i,
			(uint64)frame->FrameAddress(), (uint64)frame->InstructionPointer());

		Image* image = frame->GetImage();
		FunctionInstance* function = frame->Function();
		if (image == NULL && function == NULL) {
			printf("  ???\n");
			continue;
		}

		BString name;
		target_addr_t baseAddress;
		if (function != NULL) {
			name = function->PrettyName();
			baseAddress = function->Address();
		} else {
			name = image->Name();
			baseAddress = image->Info().TextBase();
		}

		printf("  %s + %#" B_PRIx64 "\n", name.String(),
			uint64(frame->InstructionPointer() - baseAddress));
	}
}
