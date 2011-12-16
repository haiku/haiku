/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ThreadHandler.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "Architecture.h"
#include "BreakpointManager.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "FunctionInstance.h"
#include "ImageDebugInfo.h"
#include "InstructionInfo.h"
#include "Jobs.h"
#include "MessageCodes.h"
#include "Register.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "StackTrace.h"
#include "Statement.h"
#include "Team.h"
#include "Tracing.h"
#include "Worker.h"


// step modes
enum {
	STEP_NONE,
	STEP_OVER,
	STEP_INTO,
	STEP_OUT
};


ThreadHandler::ThreadHandler(Thread* thread, Worker* worker,
	DebuggerInterface* debuggerInterface,
	BreakpointManager* breakpointManager)
	:
	fThread(thread),
	fWorker(worker),
	fDebuggerInterface(debuggerInterface),
	fBreakpointManager(breakpointManager),
	fStepMode(STEP_NONE),
	fStepStatement(NULL),
	fBreakpointAddress(0),
	fPreviousInstructionPointer(0),
	fPreviousFrameAddress(0),
	fSingleStepping(false)
{
	fDebuggerInterface->AcquireReference();
}


ThreadHandler::~ThreadHandler()
{
	_ClearContinuationState();
	fDebuggerInterface->ReleaseReference();
}


void
ThreadHandler::Init()
{
	fWorker->ScheduleJob(new(std::nothrow) GetThreadStateJob(fDebuggerInterface,
		fThread));
}


status_t
ThreadHandler::SetBreakpointAndRun(target_addr_t address)
{
	status_t error = _InstallTemporaryBreakpoint(address);
	if (error != B_OK)
		return error;

	fPreviousInstructionPointer = 0;
	resume_thread(ThreadID());
		// TODO: This should probably better be a DebuggerInterface method,
		// but this method is used only when debugging a local team anyway.
	// Pretend "step out" mode, so that the temporary breakpoint hit will not
	// be ignored.
	fStepMode = STEP_OUT;
	fSingleStepping = false;

	return B_OK;
}


bool
ThreadHandler::HandleThreadDebugged(ThreadDebuggedEvent* event)
{
	return _HandleThreadStopped(NULL, THREAD_STOPPED_DEBUGGED);
}


bool
ThreadHandler::HandleDebuggerCall(DebuggerCallEvent* event)
{
	BString message;
	fDebuggerInterface->ReadMemoryString(event->Message(), 1024, message);
	return _HandleThreadStopped(NULL, THREAD_STOPPED_DEBUGGER_CALL, message);
}


bool
ThreadHandler::HandleBreakpointHit(BreakpointHitEvent* event)
{
	CpuState* cpuState = event->GetCpuState();
	target_addr_t instructionPointer = cpuState->InstructionPointer();

	TRACE_EVENTS("ThreadHandler::HandleBreakpointHit(): ip: %llx\n",
		instructionPointer);

	// check whether this is a temporary breakpoint we're waiting for
	if (fBreakpointAddress != 0 && instructionPointer == fBreakpointAddress
		&& fStepMode != STEP_NONE) {
		if (_HandleBreakpointHitStep(cpuState))
			return true;
	} else {
		// Might be a user breakpoint, but could as well be a temporary
		// breakpoint of another thread.
		AutoLocker<Team> locker(fThread->GetTeam());
		Breakpoint* breakpoint = fThread->GetTeam()->BreakpointAtAddress(
			cpuState->InstructionPointer());
		bool continueThread = false;
		if (breakpoint == NULL) {
			// spurious breakpoint -- might be a temporary breakpoint, that has
			// already been uninstalled
			continueThread = true;
		} else if (!breakpoint->HasEnabledUserBreakpoint()) {
			// breakpoint of another thread or one that has been disabled in
			// the meantime
			continueThread = true;
		}

		if (continueThread) {
			if (fSingleStepping) {
				// We might have hit a just-installed software breakpoint and
				// thus haven't stepped at all. Just try again.
				if (fPreviousInstructionPointer == instructionPointer) {
					fDebuggerInterface->SingleStepThread(ThreadID());
					return true;
				}

				// That shouldn't happen. Try something reasonable anyway.
				if (fStepMode != STEP_NONE) {
					if (_HandleSingleStepStep(cpuState))
						return true;
				}
			}

			return false;
		}
	}

	return _HandleThreadStopped(cpuState, THREAD_STOPPED_BREAKPOINT);
}


bool
ThreadHandler::HandleWatchpointHit(WatchpointHitEvent* event)
{
	return _HandleThreadStopped(event->GetCpuState(),
		THREAD_STOPPED_WATCHPOINT);
}


bool
ThreadHandler::HandleSingleStep(SingleStepEvent* event)
{
	// Check whether we're stepping automatically.
	if (fStepMode != STEP_NONE) {
		if (_HandleSingleStepStep(event->GetCpuState()))
			return true;
	}

	return _HandleThreadStopped(event->GetCpuState(),
		THREAD_STOPPED_SINGLE_STEP);
}


bool
ThreadHandler::HandleExceptionOccurred(ExceptionOccurredEvent* event)
{
	char buffer[256];
	get_debug_exception_string(event->Exception(), buffer, sizeof(buffer));
	return _HandleThreadStopped(NULL, THREAD_STOPPED_EXCEPTION, buffer);
}


void
ThreadHandler::HandleThreadAction(uint32 action)
{
	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->State() == THREAD_STATE_UNKNOWN)
		return;

	// When stop is requested, thread must be running, otherwise stopped.
	if (action == MSG_THREAD_STOP
			? fThread->State() != THREAD_STATE_RUNNING
			: fThread->State() != THREAD_STATE_STOPPED) {
		return;
	}

	// When stepping we need a stack trace. Save it before unsetting the state.
	CpuState* cpuState = fThread->GetCpuState();
	StackTrace* stackTrace = fThread->GetStackTrace();
	BReference<CpuState> cpuStateReference(cpuState);
	BReference<StackTrace> stackTraceReference(stackTrace);

	// When continuing the thread update thread state before actually issuing
	// the command, since we need to unlock.
	if (action != MSG_THREAD_STOP) {
		_SetThreadState(THREAD_STATE_RUNNING, NULL, THREAD_STOPPED_UNKNOWN,
			BString());
	}

	locker.Unlock();

	switch (action) {
		case MSG_THREAD_RUN:
			fStepMode = STEP_NONE;
			_RunThread(0);
			return;
		case MSG_THREAD_STOP:
			fStepMode = STEP_NONE;
			fDebuggerInterface->StopThread(ThreadID());
			return;
		case MSG_THREAD_STEP_OVER:
		case MSG_THREAD_STEP_INTO:
		case MSG_THREAD_STEP_OUT:
			break;
	}

	TRACE_CONTROL("ThreadHandler::HandleThreadAction(MSG_THREAD_STEP_*)\n");

	// We want to step. We need a stack trace for that purpose. If we don't
	// have one yet, get it. Start with the CPU state.
	if (stackTrace == NULL && cpuState == NULL) {
		if (fDebuggerInterface->GetCpuState(fThread->ID(), cpuState) == B_OK)
			cpuStateReference.SetTo(cpuState, true);
	}

	if (stackTrace == NULL && cpuState != NULL) {
		if (fDebuggerInterface->GetArchitecture()->CreateStackTrace(
				fThread->GetTeam(), this, cpuState, stackTrace, 1) == B_OK) {
			stackTraceReference.SetTo(stackTrace, true);
		}
	}

	if (stackTrace == NULL || stackTrace->CountFrames() == 0) {
		_StepFallback();
		return;
	}

	StackFrame* frame = stackTrace->FrameAt(0);

	TRACE_CONTROL("  ip: %#llx\n", frame->InstructionPointer());

	// When the thread is in a syscall, do the same for all step kinds: Stop it
	// when it returns by means of a breakpoint.
	if (frame->Type() == STACK_FRAME_TYPE_SYSCALL) {
		// set a breakpoint at the CPU state's instruction pointer (points to
		// the return address, unlike the stack frame's instruction pointer)
// TODO: This is doesn't work correctly anymore. When stepping over a "syscall"
// instruction the thread is stopped twice. The after the first step the PC is
// incorrectly shown at the "syscall" instruction. Then we step again and are
// stopped at the temporary breakpoint after the "syscall" instruction. There
// are two problems. The first one is that we don't (cannot?) discriminate
// between the thread being in a syscall (like in a blocking syscall) and the
// thread having been stopped (or singled-stepped) at the end of the syscall.
// The second issue is that the temporary breakpoint is probably not necessary
// anymore, since single-stepping over "syscall" instructions should just work
// as expected.
		status_t error = _InstallTemporaryBreakpoint(
			frame->GetCpuState()->InstructionPointer());
		if (error != B_OK) {
			_StepFallback();
			return;
		}

		fStepMode = STEP_OUT;
		_RunThread(frame->GetCpuState()->InstructionPointer());
		return;
	}

	// For "step out" just set a temporary breakpoint on the return address.
	if (action == MSG_THREAD_STEP_OUT) {
		status_t error = _InstallTemporaryBreakpoint(frame->ReturnAddress());
		if (error != B_OK) {
			_StepFallback();
			return;
		}
		fPreviousFrameAddress = frame->FrameAddress();
		fStepMode = STEP_OUT;
		_RunThread(frame->GetCpuState()->InstructionPointer());
		return;
	}

	// For "step in" and "step over" we also need the source code statement at
	// the current instruction pointer.
	fStepStatement = _GetStatementAtInstructionPointer(frame);
	if (fStepStatement == NULL) {
		_StepFallback();
		return;
	}

	TRACE_CONTROL("  statement: %#llx - %#llx\n",
		fStepStatement->CoveringAddressRange().Start(),
		fStepStatement->CoveringAddressRange().End());

	if (action == MSG_THREAD_STEP_INTO) {
		// step into
		fStepMode = STEP_INTO;
		_SingleStepThread(frame->GetCpuState()->InstructionPointer());
	} else {
		fPreviousFrameAddress = frame->FrameAddress();
		// step over
		fStepMode = STEP_OVER;
		if (!_DoStepOver(frame->GetCpuState()))
			_StepFallback();
	}
}


void
ThreadHandler::HandleThreadStateChanged()
{
	AutoLocker<Team> locker(fThread->GetTeam());

	// cancel jobs for this thread
	fWorker->AbortJob(SimpleJobKey(fThread, JOB_TYPE_GET_CPU_STATE));
	fWorker->AbortJob(SimpleJobKey(fThread, JOB_TYPE_GET_STACK_TRACE));

	// If the thread is stopped and has no CPU state yet, schedule a job.
	if (fThread->State() == THREAD_STATE_STOPPED
			&& fThread->GetCpuState() == NULL) {
		fWorker->ScheduleJob(
			new(std::nothrow) GetCpuStateJob(fDebuggerInterface, fThread));
	}
}


void
ThreadHandler::HandleCpuStateChanged()
{
	AutoLocker<Team> locker(fThread->GetTeam());

	// cancel stack trace job for this thread
	fWorker->AbortJob(SimpleJobKey(fThread, JOB_TYPE_GET_STACK_TRACE));

	// If the thread has a CPU state, but no stack trace yet, schedule a job.
	if (fThread->GetCpuState() != NULL && fThread->GetStackTrace() == NULL) {
		fWorker->ScheduleJob(
			new(std::nothrow) GetStackTraceJob(fDebuggerInterface,
				fDebuggerInterface->GetArchitecture(), fThread));
	}
}


void
ThreadHandler::HandleStackTraceChanged()
{
}


status_t
ThreadHandler::GetImageDebugInfo(Image* image, ImageDebugInfo*& _info)
{
	AutoLocker<Team> teamLocker(fThread->GetTeam());

	if (image->GetImageDebugInfo() != NULL) {
		_info = image->GetImageDebugInfo();
		_info->AcquireReference();
		return B_OK;
	}

	// Let's be lazy. If the image debug info has not been loaded yet, the user
	// can't have seen any source code either.
	return B_ENTRY_NOT_FOUND;
}


bool
ThreadHandler::_HandleThreadStopped(CpuState* cpuState, uint32 stoppedReason,
	const BString& stoppedReasonInfo)
{
	_ClearContinuationState();

	AutoLocker<Team> locker(fThread->GetTeam());

	_SetThreadState(THREAD_STATE_STOPPED, cpuState, stoppedReason,
		stoppedReasonInfo);

	return true;
}


void
ThreadHandler::_SetThreadState(uint32 state, CpuState* cpuState,
	uint32 stoppedReason, const BString& stoppedReasonInfo)
{
	fThread->SetState(state, stoppedReason, stoppedReasonInfo);
	fThread->SetCpuState(cpuState);
}


Statement*
ThreadHandler::_GetStatementAtInstructionPointer(StackFrame* frame)
{
	AutoLocker<Team> locker(fThread->GetTeam());

	FunctionInstance* functionInstance = frame->Function();
	if (functionInstance == NULL)
		return NULL;
	FunctionDebugInfo* function = functionInstance->GetFunctionDebugInfo();

	// If there's source code attached to the function, we can just get the
	// statement.
//	SourceCode* sourceCode = function->GetSourceCode();
//	if (sourceCode != NULL) {
//		Statement* statement = sourceCode->StatementAtAddress(
//			frame->InstructionPointer());
//		if (statement != NULL)
//			statement->AcquireReference();
//		return statement;
//	}

	locker.Unlock();

	// We need to get the statement from the debug info of the function.
	Statement* statement;
	if (function->GetSpecificImageDebugInfo()->GetStatement(function,
			frame->InstructionPointer(), statement) != B_OK) {
		return NULL;
	}

	return statement;
}


void
ThreadHandler::_StepFallback()
{
	fStepMode = STEP_NONE;
	_SingleStepThread(0);
}


bool
ThreadHandler::_DoStepOver(CpuState* cpuState)
{
	TRACE_CONTROL("ThreadHandler::_DoStepOver()\n");

	// The basic strategy is to single-step out of the statement like for
	// "step into", only we have to avoid stepping into subroutines. Hence we
	// check whether the current instruction is a subroutine call. If not, we
	// just single-step, otherwise we set a breakpoint after the instruction.
	InstructionInfo info;
	if (fDebuggerInterface->GetArchitecture()->GetInstructionInfo(
			cpuState->InstructionPointer(), info) != B_OK) {
		TRACE_CONTROL("  failed to get instruction info\n");
		return false;
	}

	if (info.Type() != INSTRUCTION_TYPE_SUBROUTINE_CALL) {
		_SingleStepThread(cpuState->InstructionPointer());

		TRACE_CONTROL("  not a subroutine call\n");
		return true;
	}

	TRACE_CONTROL("  subroutine call -- installing breakpoint at address "
		"%#llx\n", info.Address() + info.Size());

	if (_InstallTemporaryBreakpoint(info.Address() + info.Size()) != B_OK)
		return false;

	_RunThread(cpuState->InstructionPointer());
	return true;
}


status_t
ThreadHandler::_InstallTemporaryBreakpoint(target_addr_t address)
{
	_UninstallTemporaryBreakpoint();

	status_t error = fBreakpointManager->InstallTemporaryBreakpoint(address,
		this);
	if (error != B_OK)
		return error;

	fBreakpointAddress = address;
	return B_OK;
}


void
ThreadHandler::_UninstallTemporaryBreakpoint()
{
	if (fBreakpointAddress == 0)
		return;

	fBreakpointManager->UninstallTemporaryBreakpoint(fBreakpointAddress, this);
	fBreakpointAddress = 0;
}


void
ThreadHandler::_ClearContinuationState()
{
	_UninstallTemporaryBreakpoint();

	if (fStepStatement != NULL) {
		fStepStatement->ReleaseReference();
		fStepStatement = NULL;
	}

	fStepMode = STEP_NONE;
	fSingleStepping = false;
}


void
ThreadHandler::_RunThread(target_addr_t instructionPointer)
{
	fPreviousInstructionPointer = instructionPointer;
	fDebuggerInterface->ContinueThread(ThreadID());
	fSingleStepping = false;
}


void
ThreadHandler::_SingleStepThread(target_addr_t instructionPointer)
{
	fPreviousInstructionPointer = instructionPointer;
	fDebuggerInterface->SingleStepThread(ThreadID());
	fSingleStepping = true;
}


bool
ThreadHandler::_HandleBreakpointHitStep(CpuState* cpuState)
{
	// in any case uninstall the temporary breakpoint
	_UninstallTemporaryBreakpoint();

	switch (fStepMode) {
		case STEP_OVER:
		{
			StackTrace* stackTrace = fThread->GetStackTrace();
			BReference<StackTrace> stackTraceReference(stackTrace);

			if (stackTrace == NULL && cpuState != NULL) {
				if (fDebuggerInterface->GetArchitecture()->CreateStackTrace(
						fThread->GetTeam(), this, cpuState, stackTrace, 1)
						== B_OK) {
					stackTraceReference.SetTo(stackTrace, true);
				}
			}
			if (stackTrace != NULL) {
				StackFrame* frame = stackTrace->FrameAt(0);
				// If we're not in the same frame we started in,
				// keep executing.
				if (frame != NULL && fPreviousFrameAddress
						!= stackTrace->FrameAt(0)->FrameAddress()) {
					status_t error = _InstallTemporaryBreakpoint(
						cpuState->InstructionPointer());
					if (error != B_OK)
						_StepFallback();
					else
						_RunThread(cpuState->InstructionPointer());
					return true;
				}
			}

			// If we're still in the statement, we continue single-stepping,
			// otherwise we're done.
			if (fStepStatement->ContainsAddress(
					cpuState->InstructionPointer())) {
				if (!_DoStepOver(cpuState))
					_StepFallback();
				return true;
			}
			fPreviousFrameAddress = 0;
			return false;
		}

		case STEP_INTO:
			// Should never happen -- we don't set a breakpoint in this case.
			return false;

		case STEP_OUT:
		{
			// That's the return address, so we're done in theory,
			// unless we're a recursive function. Check if we've actually
			// exited the previous stack frame or not.
			target_addr_t framePointer = cpuState->StackFramePointer();
			bool hasExitedFrame = fDebuggerInterface->GetArchitecture()
				->StackGrowthDirection() == STACK_GROWTH_DIRECTION_POSITIVE
					? framePointer < fPreviousFrameAddress
					: framePointer > fPreviousFrameAddress;

			if (!hasExitedFrame) {
				status_t error = _InstallTemporaryBreakpoint(
					cpuState->InstructionPointer());
				if (error != B_OK)
					_StepFallback();
				else
					_RunThread(cpuState->InstructionPointer());
				return true;
			}
			fPreviousFrameAddress = 0;
		}

		default:
			return false;
	}
}


bool
ThreadHandler::_HandleSingleStepStep(CpuState* cpuState)
{
	TRACE_CONTROL("ThreadHandler::_HandleSingleStepStep(): ip: %llx\n",
		cpuState->InstructionPointer());

	switch (fStepMode) {
		case STEP_INTO:
		{
			// We continue stepping as long as we're in the statement.
			if (fStepStatement->ContainsAddress(cpuState->InstructionPointer())) {
				_SingleStepThread(cpuState->InstructionPointer());
				return true;
			}

			StackTrace* stackTrace = fThread->GetStackTrace();
			BReference<StackTrace> stackTraceReference(stackTrace);

			if (stackTrace == NULL && cpuState != NULL) {
				if (fDebuggerInterface->GetArchitecture()->CreateStackTrace(
						fThread->GetTeam(), this, cpuState, stackTrace, 1)
						== B_OK) {
					stackTraceReference.SetTo(stackTrace, true);
				}
			}

			if (stackTrace != NULL) {
				StackFrame* frame = stackTrace->FrameAt(0);
				Image* image = frame->GetImage();
				ImageDebugInfo* info = NULL;
				if (GetImageDebugInfo(image, info) != B_OK)
					return false;

				BReference<ImageDebugInfo>(info, true);
				if (info->GetAddressSectionType(
						cpuState->InstructionPointer())
						== ADDRESS_SECTION_TYPE_PLT) {
					_SingleStepThread(cpuState->InstructionPointer());
					return true;
				}
			}
			return false;
		}

		case STEP_OVER:
		{
			// If we have stepped out of the statement, we're done.
			if (!fStepStatement->ContainsAddress(cpuState->InstructionPointer()))
				return false;
			return _DoStepOver(cpuState);
		}

		case STEP_OUT:
			// We never single-step in this case.
		default:
			return false;
	}
}
