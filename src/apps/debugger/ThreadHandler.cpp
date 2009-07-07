/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "StackTrace.h"
#include "Statement.h"
#include "Team.h"
#include "TeamDebugModel.h"
#include "Worker.h"


// step modes
enum {
	STEP_NONE,
	STEP_OVER,
	STEP_INTO,
	STEP_OUT
};


ThreadHandler::ThreadHandler(TeamDebugModel* debugModel, Thread* thread,
	Worker* worker, DebuggerInterface* debuggerInterface,
	BreakpointManager* breakpointManager)
	:
	fDebugModel(debugModel),
	fThread(thread),
	fWorker(worker),
	fDebuggerInterface(debuggerInterface),
	fBreakpointManager(breakpointManager),
	fStepMode(STEP_NONE),
	fStepStatement(NULL),
	fBreakpointAddress(0),
	fPreviousInstructionPointer(0),
	fSingleStepping(false)
{
}


ThreadHandler::~ThreadHandler()
{
	_ClearContinuationState();
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
	return _HandleThreadStopped(NULL);
}


bool
ThreadHandler::HandleDebuggerCall(DebuggerCallEvent* event)
{
	return _HandleThreadStopped(NULL);
}


bool
ThreadHandler::HandleBreakpointHit(BreakpointHitEvent* event)
{
	CpuState* cpuState = event->GetCpuState();
	target_addr_t instructionPointer = cpuState->InstructionPointer();

	// check whether this is a temporary breakpoint we're waiting for
	if (fBreakpointAddress != 0 && instructionPointer == fBreakpointAddress
		&& fStepMode != STEP_NONE) {
		if (_HandleBreakpointHitStep(cpuState))
			return true;
	} else {
		// Might be a user breakpoint, but could as well be a temporary
		// breakpoint of another thread.
		AutoLocker<TeamDebugModel> locker(fDebugModel);
		Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(
			cpuState->InstructionPointer());
		bool continueThread = false;
		if (breakpoint == NULL) {
			// spurious breakpoint -- might be a temporary breakpoint, that has
			// already been uninstalled
			continueThread = true;
		} else if (breakpoint->HasEnabledUserBreakpoint()) {
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

	return _HandleThreadStopped(cpuState);
}


bool
ThreadHandler::HandleWatchpointHit(WatchpointHitEvent* event)
{
	return _HandleThreadStopped(event->GetCpuState());
}


bool
ThreadHandler::HandleSingleStep(SingleStepEvent* event)
{
	// Check whether we're stepping automatically.
	if (fStepMode != STEP_NONE) {
		if (_HandleSingleStepStep(event->GetCpuState()))
			return true;
	}

	return _HandleThreadStopped(event->GetCpuState());
}


bool
ThreadHandler::HandleExceptionOccurred(ExceptionOccurredEvent* event)
{
	return _HandleThreadStopped(NULL);
}


void
ThreadHandler::HandleThreadAction(uint32 action)
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

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
	Reference<CpuState> cpuStateReference(cpuState);
	Reference<StackTrace> stackTraceReference(stackTrace);

	// When continuing the thread update thread state before actually issuing
	// the command, since we need to unlock.
	if (action != MSG_THREAD_STOP)
		_SetThreadState(THREAD_STATE_RUNNING, NULL);

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

	// We want to step. We need a stack trace for that purpose. If we don't
	// have one yet, get it. Start with the CPU state.
	if (stackTrace == NULL && cpuState == NULL) {
		if (fDebuggerInterface->GetCpuState(fThread->ID(), cpuState) == B_OK)
			cpuStateReference.SetTo(cpuState, true);
	}

	if (stackTrace == NULL && cpuState != NULL) {
		if (fDebuggerInterface->GetArchitecture()->CreateStackTrace(
				fThread->GetTeam(), this, cpuState, stackTrace) == B_OK) {
			stackTraceReference.SetTo(stackTrace, true);
		}
	}

	if (stackTrace == NULL || stackTrace->CountFrames() == 0) {
		_StepFallback();
		return;
	}

	StackFrame* frame = stackTrace->FrameAt(0);

	// When the thread is in a syscall, do the same for all step kinds: Stop it
	// when it return by means of a breakpoint.
	if (frame->Type() == STACK_FRAME_TYPE_SYSCALL) {
		// set a breakpoint at the CPU state's instruction pointer (points to
		// the return address, unlike the stack frame's instruction pointer)
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

	if (action == MSG_THREAD_STEP_INTO) {
		// step into
		fStepMode = STEP_INTO;
		_SingleStepThread(frame->GetCpuState()->InstructionPointer());
	} else {
		// step over
		fStepMode = STEP_OVER;
		if (!_DoStepOver(frame->GetCpuState()))
			_StepFallback();
	}
}


void
ThreadHandler::HandleThreadStateChanged()
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	// cancel jobs for this thread
	fWorker->AbortJob(JobKey(fThread, JOB_TYPE_GET_CPU_STATE));
	fWorker->AbortJob(JobKey(fThread, JOB_TYPE_GET_STACK_TRACE));

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
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	// cancel stack trace job for this thread
	fWorker->AbortJob(JobKey(fThread, JOB_TYPE_GET_STACK_TRACE));

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
		_info->AddReference();
		return B_OK;
	}

	// Let's be lazy. If the image debug info has not been loaded yet, the user
	// can't have seen any source code either.
	return B_ENTRY_NOT_FOUND;
}


bool
ThreadHandler::_HandleThreadStopped(CpuState* cpuState)
{
	_ClearContinuationState();

	AutoLocker<TeamDebugModel> locker(fDebugModel);

	_SetThreadState(THREAD_STATE_STOPPED, cpuState);

	return true;
}


void
ThreadHandler::_SetThreadState(uint32 state, CpuState* cpuState)
{
	fThread->SetState(state);
	fThread->SetCpuState(cpuState);
}


Statement*
ThreadHandler::_GetStatementAtInstructionPointer(StackFrame* frame)
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

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
//			statement->AddReference();
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
	// The basic strategy is to single-step out of the statement like for
	// "step into", only we have to avoid stepping into subroutines. Hence we
	// check whether the current instruction is a subroutine call. If not, we
	// just single-step, otherwise we set a breakpoint after the instruction.
	InstructionInfo info;
	if (fDebuggerInterface->GetArchitecture()->GetInstructionInfo(
			cpuState->InstructionPointer(), info) != B_OK) {
		return false;
	}

	if (info.Type() != INSTRUCTION_TYPE_SUBROUTINE_CALL) {
		_SingleStepThread(cpuState->InstructionPointer());
		return true;
	}

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
		fStepStatement->RemoveReference();
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
			// If we're still in the statement, we continue single-stepping,
			// otherwise we're done.
			if (fStepStatement->ContainsAddress(
					cpuState->InstructionPointer())) {
				_SingleStepThread(cpuState->InstructionPointer());
				return true;
			}
			return false;

		case STEP_INTO:
			// Should never happen -- we don't set a breakpoint in this case.
		case STEP_OUT:
			// That's the return address, so we're done.
		default:
			return false;
	}
}


bool
ThreadHandler::_HandleSingleStepStep(CpuState* cpuState)
{
	switch (fStepMode) {
		case STEP_INTO:
		{
			// We continue stepping as long as we're in the statement.
			if (fStepStatement->ContainsAddress(cpuState->InstructionPointer())) {
				_SingleStepThread(cpuState->InstructionPointer());
				return true;
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
