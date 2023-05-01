/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliStackTraceCommand.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "CliContext.h"
#include "StackTrace.h"
#include "Team.h"
#include "UiUtils.h"


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
	while (stackTrace == NULL) {
		context.WaitForEvent(CliContext::MSG_THREAD_STACK_TRACE_CHANGED);
		if (context.IsTerminating())
			return;
		stackTrace = thread->GetStackTrace();
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

		char functionName[512];
		UiUtils::FunctionNameForFrame(frame, functionName,
			sizeof(functionName));
		printf("  %s\n", functionName);
	}
}
