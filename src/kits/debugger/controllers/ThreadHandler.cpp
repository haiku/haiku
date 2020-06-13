/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ThreadHandler.h"

#include <stdio.h>

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Variant.h>

#include "Architecture.h"
#include "BreakpointManager.h"
#include "CpuState.h"
#include "DebugEvent.h"
#include "DebuggerInterface.h"
#include "ExpressionInfo.h"
#include "FunctionInstance.h"
#include "ImageDebugInfo.h"
#include "InstructionInfo.h"
#include "Jobs.h"
#include "MessageCodes.h"
#include "Register.h"
#include "SignalDispositionTypes.h"
#include "SourceCode.h"
#include "SourceLanguage.h"
#include "SpecificImageDebugInfo.h"
#include "StackTrace.h"
#include "Statement.h"
#include "SyntheticPrimitiveType.h"
#include "Team.h"
#include "Tracing.h"
#include "Value.h"
#include "ValueLocation.h"
#include "Worker.h"


// step modes
enum {
	STEP_NONE,
	STEP_OVER,
	STEP_INTO,
	STEP_OUT,
	STEP_UNTIL
};


class ExpressionEvaluationListener : public ExpressionInfo::Listener {
public:
	ExpressionEvaluationListener(ThreadHandler* handler)
	:
	fHandler(handler)
	{
		fHandler->AcquireReference();
	}

	~ExpressionEvaluationListener()
	{
		fHandler->ReleaseReference();
	}

	virtual void ExpressionEvaluated(ExpressionInfo* info, status_t result,
		ExpressionResult* value)
	{
		fHandler->_HandleBreakpointConditionEvaluated(value);
	}

private:
	ThreadHandler* fHandler;
};


ThreadHandler::ThreadHandler(::Thread* thread, Worker* worker,
	DebuggerInterface* debuggerInterface, JobListener* jobListener,
	BreakpointManager* breakpointManager)
	:
	fThread(thread),
	fWorker(worker),
	fDebuggerInterface(debuggerInterface),
	fJobListener(jobListener),
	fBreakpointManager(breakpointManager),
	fStepMode(STEP_NONE),
	fStepStatement(NULL),
	fBreakpointAddress(0),
	fSteppedOverFunctionAddress(0),
	fPreviousInstructionPointer(0),
	fPreviousFrameAddress(0),
	fSingleStepping(false),
	fConditionWaitSem(-1),
	fConditionResult(NULL)
{
	fDebuggerInterface->AcquireReference();
}


ThreadHandler::~ThreadHandler()
{
	_ClearContinuationState();
	fDebuggerInterface->ReleaseReference();

	if (fConditionWaitSem > 0)
		delete_sem(fConditionWaitSem);
}


void
ThreadHandler::Init()
{
	fWorker->ScheduleJob(new(std::nothrow) GetThreadStateJob(fDebuggerInterface,
		fThread), fJobListener);
	fConditionWaitSem = create_sem(0, "breakpoint condition waiter");
}


status_t
ThreadHandler::SetBreakpointAndRun(target_addr_t address)
{
	status_t error = _InstallTemporaryBreakpoint(address);
	if (error != B_OK)
		return error;

	fPreviousInstructionPointer = 0;
	fDebuggerInterface->ContinueThread(ThreadID());

	// Pretend "step out" mode, so that the temporary breakpoint hit will not
	// be ignored.
	fStepMode = STEP_OUT;
	fSingleStepping = false;

	return B_OK;
}


bool
ThreadHandler::HandleThreadDebugged(ThreadDebuggedEvent* event,
	const BString& stoppedReason)
{
	return _HandleThreadStopped(NULL, THREAD_STOPPED_DEBUGGED, stoppedReason);
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

	TRACE_EVENTS("ThreadHandler::HandleBreakpointHit(): ip: %" B_PRIx64 "\n",
		instructionPointer);

	// check whether this is a temporary breakpoint we're waiting for
	if (fBreakpointAddress != 0 && instructionPointer == fBreakpointAddress
		&& fStepMode != STEP_NONE) {
		if (fStepMode != STEP_UNTIL && _HandleBreakpointHitStep(cpuState))
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
		} else {
			locker.Unlock();
			if (_HandleBreakpointConditionIfNeeded(cpuState))
				return true;

			locker.Lock();
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


bool
ThreadHandler::HandleSignalReceived(SignalReceivedEvent* event)
{
	::Team* team = fThread->GetTeam();
	AutoLocker<Team> locker(team);

	const SignalInfo& info = event->GetSignalInfo();
	int32 signal = info.Signal();
	int32 disposition = team->SignalDispositionFor(signal);

	switch (disposition) {
		case SIGNAL_DISPOSITION_IGNORE:
			return false;
		case SIGNAL_DISPOSITION_STOP_AT_SIGNAL_HANDLER:
		{
			const struct sigaction& handlerInfo = info.Handler();
			target_addr_t address = 0;
			if ((handlerInfo.sa_flags & SA_SIGINFO) != 0)
				address = (target_addr_t)handlerInfo.sa_sigaction;
			else
				address = (target_addr_t)handlerInfo.sa_handler;

			if (address == (target_addr_t)SIG_DFL
				|| address == (target_addr_t)SIG_IGN
				|| address == (target_addr_t)SIG_HOLD) {
				address = 0;
			}

			if (address != 0 && _InstallTemporaryBreakpoint(address) == B_OK
				&& fDebuggerInterface->ContinueThread(ThreadID()) == B_OK) {
				fStepMode = STEP_UNTIL;
				return true;
			}

			// fall through if no handler or if we failed to
			// set a breakpoint at the handler
		}
		case SIGNAL_DISPOSITION_STOP_AT_RECEIPT:
		{
			BString stopReason;
			stopReason.SetToFormat("Received signal %" B_PRId32 " (%s)",
				signal, strsignal(signal));
			return _HandleThreadStopped(NULL, THREAD_STOPPED_DEBUGGED,
				stopReason);
		}
		default:
			break;
	}

	return false;
}


void
ThreadHandler::HandleThreadAction(uint32 action, target_addr_t address)
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

	if (action == MSG_THREAD_SET_ADDRESS) {
		_HandleSetAddress(cpuState, address);
		return;
	}

	// When continuing the thread update thread state before actually issuing
	// the command, since we need to unlock.
	if (action != MSG_THREAD_STOP) {
		_SetThreadState(THREAD_STATE_RUNNING, NULL, THREAD_STOPPED_UNKNOWN,
			BString());
	}

	locker.Unlock();

	switch (action) {
		case MSG_THREAD_RUN:
			fStepMode = address != 0 ? STEP_UNTIL : STEP_NONE;
			if (address != 0)
				_InstallTemporaryBreakpoint(address);
			_RunThread(0);
			return;
		case MSG_THREAD_STOP:
			fStepMode = STEP_NONE;
			if (fDebuggerInterface->StopThread(ThreadID()) == B_OK)
				fThread->SetStopRequestPending();
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
				fThread->GetTeam(), this, cpuState, stackTrace, NULL, 1,
				false, false) == B_OK) {
			stackTraceReference.SetTo(stackTrace, true);
		}
	}

	if (stackTrace == NULL || stackTrace->CountFrames() == 0) {
		_StepFallback();
		return;
	}

	StackFrame* frame = stackTrace->FrameAt(0);

	TRACE_CONTROL("  ip: %#" B_PRIx64 "\n", frame->InstructionPointer());

	target_addr_t frameIP = frame->GetCpuState()->InstructionPointer();
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
		status_t error = _InstallTemporaryBreakpoint(frameIP);
		if (error != B_OK) {
			_StepFallback();
			return;
		}

		fStepMode = STEP_OUT;
		_RunThread(frameIP);
		return;
	}

	// For "step out" just set a temporary breakpoint on the return address.
	if (action == MSG_THREAD_STEP_OUT) {
		status_t error = _InstallTemporaryBreakpoint(frame->ReturnAddress());
		if (error != B_OK) {
			_StepFallback();
			return;
		}
		fPreviousInstructionPointer = frameIP;
		fPreviousFrameAddress = frame->FrameAddress();
		fStepMode = STEP_OUT;
		_RunThread(frameIP);
		return;
	}

	// For "step in" and "step over" we also need the source code statement at
	// the current instruction pointer.
	fStepStatement = _GetStatementAtInstructionPointer(frame);
	if (fStepStatement == NULL) {
		_StepFallback();
		return;
	}

	TRACE_CONTROL("  statement: %#" B_PRIx64 " - %#" B_PRIx64 "\n",
		fStepStatement->CoveringAddressRange().Start(),
		fStepStatement->CoveringAddressRange().End());

	if (action == MSG_THREAD_STEP_INTO) {
		// step into
		fStepMode = STEP_INTO;
		_SingleStepThread(frameIP);
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
			new(std::nothrow) GetCpuStateJob(fDebuggerInterface, fThread),
			fJobListener);
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
				fJobListener, fDebuggerInterface->GetArchitecture(),
				fThread), fJobListener);
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


bool
ThreadHandler::_HandleSetAddress(CpuState* state, target_addr_t address)
{
	CpuState* newState = NULL;
	if (state->Clone(newState) != B_OK)
		return false;
	BReference<CpuState> stateReference(newState, true);

	newState->SetInstructionPointer(address);
	if (fDebuggerInterface->SetCpuState(fThread->ID(), newState) != B_OK)
		return false;

	AutoLocker<Team> locker(fThread->GetTeam());
	fThread->SetStackTrace(NULL);
	fThread->SetCpuState(newState);

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
			cpuState->InstructionPointer(), info, cpuState) != B_OK) {
		TRACE_CONTROL("  failed to get instruction info\n");
		return false;
	}

	if (info.Type() != INSTRUCTION_TYPE_SUBROUTINE_CALL) {
		_SingleStepThread(cpuState->InstructionPointer());

		TRACE_CONTROL("  not a subroutine call\n");
		return true;
	}

	TRACE_CONTROL("  subroutine call -- installing breakpoint at address "
		"%#" B_PRIx64 "\n", info.Address() + info.Size());

	if (_InstallTemporaryBreakpoint(info.Address() + info.Size()) != B_OK)
		return false;

	fSteppedOverFunctionAddress = info.TargetAddress();

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
						fThread->GetTeam(), this, cpuState, stackTrace, NULL,
						1, false, false) == B_OK) {
					stackTraceReference.SetTo(stackTrace, true);
				}
			}
			if (stackTrace != NULL) {
				StackFrame* frame = stackTrace->FrameAt(0);
				// If we're not in the same frame we started in,
				// keep executing.
				if (frame != NULL && fPreviousFrameAddress
						!= frame->FrameAddress()) {
					status_t error = _InstallTemporaryBreakpoint(
						cpuState->InstructionPointer());
					if (error != B_OK)
						_StepFallback();
					else
						_RunThread(cpuState->InstructionPointer());
					return true;
				}
			}

			if (fPreviousFrameAddress != 0 && fSteppedOverFunctionAddress != 0
					&& fSteppedOverFunctionAddress != cpuState->InstructionPointer()) {
				TRACE_CONTROL("STEP_OVER: called function address %#" B_PRIx64
					", previous frame address: %#" B_PRIx64 ", frame address: %#"
					B_PRIx64 ", adding return info\n", fSteppedOverFunctionAddress,
					fPreviousFrameAddress, stackTrace->FrameAt(0)->FrameAddress());
				ReturnValueInfo* returnInfo = new(std::nothrow) ReturnValueInfo(
					fSteppedOverFunctionAddress, cpuState);
				if (returnInfo == NULL)
					return false;

				BReference<ReturnValueInfo> returnInfoReference(returnInfo, true);

				if (fThread->AddReturnValueInfo(returnInfo) != B_OK)
					return false;

				returnInfoReference.Detach();
				fSteppedOverFunctionAddress = 0;
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
			// exited the previous stack frame or not
			if (!_HasExitedFrame(cpuState->StackFramePointer())) {
				status_t error = _InstallTemporaryBreakpoint(
					cpuState->InstructionPointer());
				if (error != B_OK)
					_StepFallback();
				else
					_RunThread(cpuState->InstructionPointer());
				return true;
			}

			if (fPreviousFrameAddress == 0)
				return false;

			TRACE_CONTROL("ThreadHandler::_HandleBreakpointHitStep() - "
				"frame pointer 0x%#" B_PRIx64 ", previous: 0x%#" B_PRIx64
				" - step out adding return value\n", cpuState
					->StackFramePointer(), fPreviousFrameAddress);
			ReturnValueInfo* info = new(std::nothrow) ReturnValueInfo(
				fPreviousInstructionPointer, cpuState);
			if (info == NULL)
				return false;
			BReference<ReturnValueInfo> infoReference(info, true);
			if (fThread->AddReturnValueInfo(info) != B_OK)
				return false;

			infoReference.Detach();
			fPreviousFrameAddress = 0;
		}

		default:
			return false;
	}
}


bool
ThreadHandler::_HandleSingleStepStep(CpuState* cpuState)
{
	TRACE_CONTROL("ThreadHandler::_HandleSingleStepStep(): ip: %" B_PRIx64 "\n",
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
						fThread->GetTeam(), this, cpuState, stackTrace, NULL,
						1, false, false) == B_OK) {
					stackTraceReference.SetTo(stackTrace, true);
				}
			}

			if (stackTrace != NULL) {
				StackFrame* frame = stackTrace->FrameAt(0);
				Image* image = frame->GetImage();
				if (image == NULL)
					return false;

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
			if (!fStepStatement->ContainsAddress(cpuState->InstructionPointer())) {
				StackTrace* stackTrace = fThread->GetStackTrace();
				BReference<StackTrace> stackTraceReference(stackTrace);
				if (stackTrace == NULL && cpuState != NULL) {
					if (fDebuggerInterface->GetArchitecture()->CreateStackTrace(
							fThread->GetTeam(), this, cpuState, stackTrace,
							NULL, 1, false, false) == B_OK) {
						stackTraceReference.SetTo(stackTrace, true);
					}
				}


				if (stackTrace != NULL) {
					if (_HasExitedFrame(stackTrace->FrameAt(0)
						->FrameAddress())) {
						TRACE_CONTROL("ThreadHandler::_HandleSingleStepStep() "
							" - adding return value for STEP_OVER\n");
						ReturnValueInfo* info = new(std::nothrow)
							ReturnValueInfo(fStepStatement
								->CoveringAddressRange().Start(), cpuState);
						if (info == NULL)
							return false;
						BReference<ReturnValueInfo> infoReference(info, true);
						if (fThread->AddReturnValueInfo(info) != B_OK)
							return false;

						infoReference.Detach();
					}
				}
				return false;
			}
			return _DoStepOver(cpuState);
		}

		case STEP_OUT:
			// We never single-step in this case.
		default:
			return false;
	}
}


bool
ThreadHandler::_HandleBreakpointConditionIfNeeded(CpuState* cpuState)
{
	AutoLocker< ::Team> teamLocker(fThread->GetTeam());
	Breakpoint* breakpoint = fThread->GetTeam()->BreakpointAtAddress(
		cpuState->InstructionPointer());

	if (breakpoint == NULL)
		return false;

	if (!breakpoint->HasEnabledUserBreakpoint())
		return false;

	const UserBreakpointInstanceList& breakpoints
		= breakpoint->UserBreakpoints();

	for (UserBreakpointInstanceList::ConstIterator it
			= breakpoints.GetIterator(); it.HasNext();) {
		UserBreakpoint* userBreakpoint = it.Next()->GetUserBreakpoint();
		if (!userBreakpoint->IsValid())
			continue;
		if (!userBreakpoint->IsEnabled())
			continue;
		if (!userBreakpoint->HasCondition())
			continue;

		StackTrace* stackTrace = fThread->GetStackTrace();
		BReference<StackTrace> stackTraceReference;
		if (stackTrace == NULL) {
			if (fDebuggerInterface->GetArchitecture()->CreateStackTrace(
				fThread->GetTeam(), this, cpuState, stackTrace, NULL, 1,
				false, true) == B_OK) {
				stackTraceReference.SetTo(stackTrace, true);
			} else
				return false;
		}

		StackFrame* frame = stackTrace->FrameAt(0);
		FunctionDebugInfo* info = frame->Function()->GetFunctionDebugInfo();
		if (info == NULL)
			return false;

		SpecificImageDebugInfo* specificInfo
			= info->GetSpecificImageDebugInfo();
		if (specificInfo == NULL)
			return false;

		SourceLanguage* language;
		if (specificInfo->GetSourceLanguage(info, language) != B_OK)
			return false;

		BReference<SourceLanguage> reference(language, true);
		ExpressionEvaluationListener* listener
			= new(std::nothrow) ExpressionEvaluationListener(this);
		if (listener == NULL)
			return false;

		ExpressionInfo* expressionInfo = new(std::nothrow) ExpressionInfo(
			userBreakpoint->Condition());

		if (expressionInfo == NULL) {
			delete listener;
			return false;
		}

		BReference<ExpressionInfo> expressionReference(expressionInfo, true);

		expressionInfo->AddListener(listener);

		status_t error = fWorker->ScheduleJob(
			new(std::nothrow) ExpressionEvaluationJob(fThread->GetTeam(),
				fDebuggerInterface, language, expressionInfo, frame, fThread),
			fJobListener);

		BPrivate::ObjectDeleter<ExpressionEvaluationListener> deleter(
			listener);
		if (error == B_OK) {
			teamLocker.Unlock();
			do {
				error = acquire_sem(fConditionWaitSem);
			} while (error == B_INTERRUPTED);

			teamLocker.Lock();

			if (_CheckStopCondition()) {
				if (fConditionResult != NULL) {
					fConditionResult->ReleaseReference();
					fConditionResult = NULL;
				}
				_SetThreadState(THREAD_STATE_STOPPED, cpuState,
					THREAD_STOPPED_BREAKPOINT, BString());
				return false;
			} else {
				fDebuggerInterface->ContinueThread(ThreadID());
				return true;
			}
		}
	}

	return false;
}


void
ThreadHandler::_HandleBreakpointConditionEvaluated(ExpressionResult* value)
{
	fConditionResult = value;
	if (fConditionResult != NULL)
		fConditionResult->AcquireReference();
	release_sem(fConditionWaitSem);
}


bool
ThreadHandler::_CheckStopCondition()
{
	// if we we're unable to properly assess the expression result
	// in any way, fall back to behaving like an unconditional breakpoint.
	if (fConditionResult == NULL)
		return true;

	if (fConditionResult->Kind() != EXPRESSION_RESULT_KIND_PRIMITIVE)
		return true;

	BVariant value;
	if (!fConditionResult->PrimitiveValue()->ToVariant(value))
		return true;

	return value.ToBool();
}


bool
ThreadHandler::_HasExitedFrame(target_addr_t framePointer) const
{
	return fDebuggerInterface->GetArchitecture()->StackGrowthDirection()
			== STACK_GROWTH_DIRECTION_POSITIVE
				? framePointer < fPreviousFrameAddress
				: framePointer > fPreviousFrameAddress;
}
