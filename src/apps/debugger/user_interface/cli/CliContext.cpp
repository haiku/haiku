/*
 * Copyright 2012-2016, Rene Gollent, rene@gollent.com.
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliContext.h"

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "StackTrace.h"
#include "UserInterface.h"
#include "ValueNodeManager.h"
#include "Variable.h"


// NOTE: This is a simple work-around for EditLine not having any kind of user
// data field. Hence in _GetPrompt() we don't have access to the context object.
// ATM only one CLI is possible in Debugger, so a static variable works well
// enough. Should that ever change, we would need a thread-safe
// EditLine* -> CliContext* map.
static CliContext* sCurrentContext;


// #pragma mark - Event


struct CliContext::Event : DoublyLinkedListLinkImpl<CliContext::Event> {
	Event(int type, Thread* thread = NULL, TeamMemoryBlock* block = NULL,
		ExpressionInfo* info = NULL, status_t expressionResult = B_OK,
		ExpressionResult* expressionValue = NULL)
		:
		fType(type),
		fThreadReference(thread),
		fMemoryBlockReference(block),
		fExpressionInfo(info),
		fExpressionResult(expressionResult),
		fExpressionValue(expressionValue)
	{
	}

	int Type() const
	{
		return fType;
	}

	Thread* GetThread() const
	{
		return fThreadReference.Get();
	}

	TeamMemoryBlock* GetMemoryBlock() const
	{
		return fMemoryBlockReference.Get();
	}

	ExpressionInfo* GetExpressionInfo() const
	{
		return fExpressionInfo;
	}

	status_t GetExpressionResult() const
	{
		return fExpressionResult;
	}

	ExpressionResult* GetExpressionValue() const
	{
		return fExpressionValue.Get();
	}


private:
	int					fType;
	BReference<Thread>	fThreadReference;
	BReference<TeamMemoryBlock> fMemoryBlockReference;
	BReference<ExpressionInfo> fExpressionInfo;
	status_t			fExpressionResult;
	BReference<ExpressionResult> fExpressionValue;
};


// #pragma mark - CliContext


CliContext::CliContext()
	:
	fLock("CliContext"),
	fTeam(NULL),
	fListener(NULL),
	fNodeManager(NULL),
	fEditLine(NULL),
	fHistory(NULL),
	fPrompt(NULL),
	fBlockingSemaphore(-1),
	fInputLoopWaitingForEvents(0),
	fEventsOccurred(0),
	fInputLoopWaiting(false),
	fTerminating(false),
	fCurrentThread(NULL),
	fCurrentStackTrace(NULL),
	fCurrentStackFrameIndex(-1),
	fCurrentBlock(NULL),
	fExpressionInfo(NULL),
	fExpressionResult(B_OK),
	fExpressionValue(NULL)
{
	sCurrentContext = this;
}


CliContext::~CliContext()
{
	Cleanup();
	sCurrentContext = NULL;

	if (fBlockingSemaphore >= 0)
		delete_sem(fBlockingSemaphore);
}


status_t
CliContext::Init(Team* team, UserInterfaceListener* listener)
{
	fTeam = team;
	fListener = listener;

	fTeam->AddListener(this);

	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	fBlockingSemaphore = create_sem(0, "CliContext block");
	if (fBlockingSemaphore < 0)
		return fBlockingSemaphore;

	fEditLine = el_init("Debugger", stdin, stdout, stderr);
	if (fEditLine == NULL)
		return B_ERROR;

	fHistory = history_init();
	if (fHistory == NULL)
		return B_ERROR;

	HistEvent historyEvent;
	history(fHistory, &historyEvent, H_SETSIZE, 100);

	el_set(fEditLine, EL_HIST, &history, fHistory);
	el_set(fEditLine, EL_EDITOR, "emacs");
	el_set(fEditLine, EL_PROMPT, &_GetPrompt);

	fNodeManager = new(std::nothrow) ValueNodeManager();
	if (fNodeManager == NULL)
		return B_NO_MEMORY;
	fNodeManager->AddListener(this);

	fExpressionInfo = new(std::nothrow) ExpressionInfo();
	if (fExpressionInfo == NULL)
		return B_NO_MEMORY;
	fExpressionInfo->AddListener(this);

	return B_OK;
}


void
CliContext::Cleanup()
{
	Terminating();

	while (Event* event = fPendingEvents.RemoveHead())
		delete event;

	if (fEditLine != NULL) {
		el_end(fEditLine);
		fEditLine = NULL;
	}

	if (fHistory != NULL) {
		history_end(fHistory);
		fHistory = NULL;
	}

	if (fTeam != NULL) {
		fTeam->RemoveListener(this);
		fTeam = NULL;
	}

	if (fNodeManager != NULL) {
		fNodeManager->ReleaseReference();
		fNodeManager = NULL;
	}

	if (fCurrentBlock != NULL) {
		fCurrentBlock->ReleaseReference();
		fCurrentBlock = NULL;
	}

	if (fExpressionInfo != NULL) {
		fExpressionInfo->ReleaseReference();
		fExpressionInfo = NULL;
	}
}


void
CliContext::Terminating()
{
	AutoLocker<BLocker> locker(fLock);

	fTerminating = true;
	_SignalInputLoop(EVENT_QUIT);

	// TODO: Signal the input loop, should it be in PromptUser()!
}


thread_id
CliContext::CurrentThreadID() const
{
	return fCurrentThread != NULL ? fCurrentThread->ID() : -1;
}


void
CliContext::SetCurrentThread(Thread* thread)
{
	AutoLocker<BLocker> locker(fLock);

	if (fCurrentThread != NULL)
		fCurrentThread->ReleaseReference();

	fCurrentThread = thread;

	if (fCurrentStackTrace != NULL) {
		fCurrentStackTrace->ReleaseReference();
		fCurrentStackTrace = NULL;
		fCurrentStackFrameIndex = -1;
		fNodeManager->SetStackFrame(NULL, NULL);
	}

	if (fCurrentThread != NULL) {
		fCurrentThread->AcquireReference();
		StackTrace* stackTrace = fCurrentThread->GetStackTrace();
		// if the thread's stack trace has already been loaded,
		// set it, otherwise we'll set it when we process the thread's
		// stack trace changed event.
		if (stackTrace != NULL) {
			fCurrentStackTrace = stackTrace;
			fCurrentStackTrace->AcquireReference();
			SetCurrentStackFrameIndex(0);
		}
	}
}


void
CliContext::PrintCurrentThread()
{
	AutoLocker<Team> teamLocker(fTeam);

	if (fCurrentThread != NULL) {
		printf("current thread: %" B_PRId32 " \"%s\"\n", fCurrentThread->ID(),
			fCurrentThread->Name());
	} else
		printf("no current thread\n");
}


void
CliContext::SetCurrentStackFrameIndex(int32 index)
{
	AutoLocker<BLocker> locker(fLock);

	if (fCurrentStackTrace == NULL)
		return;
	else if (index < 0 || index >= fCurrentStackTrace->CountFrames())
		return;

	fCurrentStackFrameIndex = index;

	StackFrame* frame = fCurrentStackTrace->FrameAt(index);
	if (frame != NULL)
		fNodeManager->SetStackFrame(fCurrentThread, frame);
}


const char*
CliContext::PromptUser(const char* prompt)
{
	fPrompt = prompt;

	int count;
	const char* line = el_gets(fEditLine, &count);

	fPrompt = NULL;

	ProcessPendingEvents();

	return line;
}


void
CliContext::AddLineToInputHistory(const char* line)
{
	HistEvent historyEvent;
	history(fHistory, &historyEvent, H_ENTER, line);
}


void
CliContext::QuitSession(bool killTeam)
{
	_PrepareToWaitForEvents(EVENT_QUIT);

	fListener->UserInterfaceQuitRequested(
		killTeam
			? UserInterfaceListener::QUIT_OPTION_ASK_KILL_TEAM
			: UserInterfaceListener::QUIT_OPTION_ASK_RESUME_TEAM);

	_WaitForEvents();
}


void
CliContext::WaitForThreadOrUser()
{
	ProcessPendingEvents();

// TODO: Deal with SIGINT as well!
	for (;;) {
		_PrepareToWaitForEvents(
			EVENT_USER_INTERRUPT | EVENT_THREAD_STOPPED);

		// check whether there are any threads stopped already
		Thread* stoppedThread = NULL;
		BReference<Thread> stoppedThreadReference;

		AutoLocker<Team> teamLocker(fTeam);

		for (ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
				Thread* thread = it.Next();) {
			if (thread->State() == THREAD_STATE_STOPPED) {
				stoppedThread = thread;
				stoppedThreadReference.SetTo(thread);
				break;
			}
		}

		teamLocker.Unlock();

		if (stoppedThread != NULL) {
			if (fCurrentThread == NULL)
				SetCurrentThread(stoppedThread);

			_SignalInputLoop(EVENT_THREAD_STOPPED);
		}

		uint32 events = _WaitForEvents();
		if ((events & EVENT_QUIT) != 0 || stoppedThread != NULL) {
			ProcessPendingEvents();
			return;
		}
	}
}


void
CliContext::WaitForEvents(int32 eventMask)
{
	for (;;) {
		_PrepareToWaitForEvents(eventMask | EVENT_USER_INTERRUPT);
		uint32 events = fEventsOccurred;
		if ((events & eventMask) == 0) {
			events = _WaitForEvents();
		}

		if ((events & EVENT_QUIT) != 0 || (events & eventMask) != 0) {
			_SignalInputLoop(eventMask);
			ProcessPendingEvents();
			return;
		}
	}
}


void
CliContext::ProcessPendingEvents()
{
	AutoLocker<Team> teamLocker(fTeam);

	for (;;) {
		// get the next event
		AutoLocker<BLocker> locker(fLock);
		Event* event = fPendingEvents.RemoveHead();
		locker.Unlock();
		if (event == NULL)
			break;
		ObjectDeleter<Event> eventDeleter(event);

		// process the event
		Thread* thread = event->GetThread();

		switch (event->Type()) {
			case EVENT_QUIT:
			case EVENT_DEBUG_REPORT_CHANGED:
			case EVENT_USER_INTERRUPT:
				break;
			case EVENT_THREAD_ADDED:
				printf("[new thread: %" B_PRId32 " \"%s\"]\n", thread->ID(),
					thread->Name());
				break;
			case EVENT_THREAD_REMOVED:
				printf("[thread terminated: %" B_PRId32 " \"%s\"]\n",
					thread->ID(), thread->Name());
				break;
			case EVENT_THREAD_STOPPED:
				printf("[thread stopped: %" B_PRId32 " \"%s\"]\n",
					thread->ID(), thread->Name());
				break;
			case EVENT_THREAD_STACK_TRACE_CHANGED:
				if (thread == fCurrentThread) {
					fCurrentStackTrace = thread->GetStackTrace();
					fCurrentStackTrace->AcquireReference();
					SetCurrentStackFrameIndex(0);
				}
				break;
			case EVENT_TEAM_MEMORY_BLOCK_RETRIEVED:
				if (fCurrentBlock != NULL) {
					fCurrentBlock->ReleaseReference();
					fCurrentBlock = NULL;
				}
				fCurrentBlock = event->GetMemoryBlock();
				break;
			case EVENT_EXPRESSION_EVALUATED:
				fExpressionResult = event->GetExpressionResult();
				if (fExpressionValue != NULL) {
					fExpressionValue->ReleaseReference();
					fExpressionValue = NULL;
				}
				fExpressionValue = event->GetExpressionValue();
				if (fExpressionValue != NULL)
					fExpressionValue->AcquireReference();
				break;
		}
	}
}


void
CliContext::ThreadAdded(const Team::ThreadEvent& threadEvent)
{
	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_ADDED, threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_ADDED);
}


void
CliContext::ThreadRemoved(const Team::ThreadEvent& threadEvent)
{
	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_REMOVED, threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_REMOVED);
}


void
CliContext::ThreadStateChanged(const Team::ThreadEvent& threadEvent)
{
	if (threadEvent.GetThread()->State() != THREAD_STATE_STOPPED)
		return;

	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_STOPPED, threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_STOPPED);
}


void
CliContext::ThreadStackTraceChanged(const Team::ThreadEvent& threadEvent)
{
	if (threadEvent.GetThread()->State() != THREAD_STATE_STOPPED)
		return;

	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_STACK_TRACE_CHANGED,
			threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_STACK_TRACE_CHANGED);
}


void
CliContext::ExpressionEvaluated(ExpressionInfo* info, status_t result,
	ExpressionResult* value)
{
	_QueueEvent(
		new(std::nothrow) Event(EVENT_EXPRESSION_EVALUATED,
			NULL, NULL, info, result, value));
	_SignalInputLoop(EVENT_EXPRESSION_EVALUATED);
}


void
CliContext::DebugReportChanged(const Team::DebugReportEvent& event)
{
	if (event.GetFinalStatus() == B_OK) {
		printf("Successfully saved debug report to %s\n",
			event.GetReportPath());
	} else {
		fprintf(stderr, "Failed to write debug report: %s\n", strerror(
				event.GetFinalStatus()));
	}

	_QueueEvent(new(std::nothrow) Event(EVENT_DEBUG_REPORT_CHANGED));
	_SignalInputLoop(EVENT_DEBUG_REPORT_CHANGED);
}


void
CliContext::CoreFileChanged(const Team::CoreFileChangedEvent& event)
{
	printf("Successfully saved core file to %s\n",
		event.GetTargetPath());

	_QueueEvent(new(std::nothrow) Event(EVENT_CORE_FILE_CHANGED));
	_SignalInputLoop(EVENT_CORE_FILE_CHANGED);
}


void
CliContext::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
	_QueueEvent(
		new(std::nothrow) Event(EVENT_TEAM_MEMORY_BLOCK_RETRIEVED,
			NULL, block));
	_SignalInputLoop(EVENT_TEAM_MEMORY_BLOCK_RETRIEVED);
}


void
CliContext::ValueNodeChanged(ValueNodeChild* nodeChild, ValueNode* oldNode,
	ValueNode* newNode)
{
	_SignalInputLoop(EVENT_VALUE_NODE_CHANGED);
}


void
CliContext::ValueNodeChildrenCreated(ValueNode* node)
{
	_SignalInputLoop(EVENT_VALUE_NODE_CHANGED);
}


void
CliContext::ValueNodeChildrenDeleted(ValueNode* node)
{
	_SignalInputLoop(EVENT_VALUE_NODE_CHANGED);
}


void
CliContext::ValueNodeValueChanged(ValueNode* oldNode)
{
	_SignalInputLoop(EVENT_VALUE_NODE_CHANGED);
}


void
CliContext::_QueueEvent(Event* event)
{
	if (event == NULL) {
		// no memory -- can't do anything about it
		return;
	}

	AutoLocker<BLocker> locker(fLock);
	fPendingEvents.Add(event);
}


void
CliContext::_PrepareToWaitForEvents(uint32 eventMask)
{
	// Set the events we're going to wait for -- always wait for "quit".
	AutoLocker<BLocker> locker(fLock);
	fInputLoopWaitingForEvents = eventMask | EVENT_QUIT;
	fEventsOccurred = fTerminating ? EVENT_QUIT : 0;
}


uint32
CliContext::_WaitForEvents()
{
	AutoLocker<BLocker> locker(fLock);

	if (fEventsOccurred == 0) {
		sem_id blockingSemaphore = fBlockingSemaphore;
		fInputLoopWaiting = true;

		locker.Unlock();

		while (acquire_sem(blockingSemaphore) == B_INTERRUPTED) {
		}

		locker.Lock();
	}

	uint32 events = fEventsOccurred;
	fEventsOccurred = 0;
	return events;
}


void
CliContext::_SignalInputLoop(uint32 events)
{
	AutoLocker<BLocker> locker(fLock);

	if ((fInputLoopWaitingForEvents & events) == 0)
		return;

	fEventsOccurred = fInputLoopWaitingForEvents & events;
	fInputLoopWaitingForEvents = 0;

	if (fInputLoopWaiting) {
		fInputLoopWaiting = false;
		release_sem(fBlockingSemaphore);
	}
}


/*static*/ const char*
CliContext::_GetPrompt(EditLine* editLine)
{
	return sCurrentContext != NULL ? sCurrentContext->fPrompt : NULL;
}
