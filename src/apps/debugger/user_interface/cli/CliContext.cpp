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
#include "Value.h"
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
	Event(int type, ::Thread* thread = NULL, TeamMemoryBlock* block = NULL,
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

	::Thread* GetThread() const
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
	BReference< ::Thread>	fThreadReference;
	BReference<TeamMemoryBlock> fMemoryBlockReference;
	BReference<ExpressionInfo> fExpressionInfo;
	status_t			fExpressionResult;
	BReference<ExpressionResult> fExpressionValue;
};


// #pragma mark - CliContext


CliContext::CliContext()
	:
	BLooper("CliContext"),
	fLock("CliContext"),
	fTeam(NULL),
	fListener(NULL),
	fNodeManager(NULL),
	fEditLine(NULL),
	fHistory(NULL),
	fPrompt(NULL),
	fWaitForEventSemaphore(-1),
	fEventOccurred(0),
	fTerminating(false),
	fStoppedThread(NULL),
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

	if (fWaitForEventSemaphore >= 0)
		delete_sem(fWaitForEventSemaphore);
}


status_t
CliContext::Init(::Team* team, UserInterfaceListener* listener)
{
	AutoLocker<BLocker> locker(fLock);

	fTeam = team;
	fListener = listener;

	fTeam->AddListener(this);

	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	fWaitForEventSemaphore = create_sem(0, "CliContext wait for event");
	if (fWaitForEventSemaphore < 0)
		return fWaitForEventSemaphore;

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
	AutoLocker<BLocker> locker(fLock);
	Terminating();

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


// TODO: Use the lifecycle methods of BLooper instead
void
CliContext::Terminating()
{
	AutoLocker<BLocker> locker(fLock);

	fTerminating = true;

	BMessage message(MSG_QUIT);
	PostMessage(&message);

	// TODO: Signal the input loop, should it be in PromptUser()!
}


thread_id
CliContext::CurrentThreadID() const
{
	AutoLocker<BLocker> locker(fLock);
	return fCurrentThread != NULL ? fCurrentThread->ID() : -1;
}


void
CliContext::SetCurrentThread(::Thread* thread)
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
	AutoLocker< ::Team> teamLocker(fTeam);
	AutoLocker<BLocker> locker(fLock);

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


status_t
CliContext::EvaluateExpression(const char* expression,
		SourceLanguage* language, target_addr_t& address)
{
	AutoLocker<BLocker> locker(fLock);
	fExpressionInfo->SetTo(expression);

	fListener->ExpressionEvaluationRequested(
		language, fExpressionInfo);
	_WaitForEvent(MSG_EXPRESSION_EVALUATED);
	if (fTerminating)
		return B_INTERRUPTED;

	BString errorMessage;
	if (fExpressionValue != NULL) {
		if (fExpressionValue->Kind() == EXPRESSION_RESULT_KIND_PRIMITIVE) {
			Value* value = fExpressionValue->PrimitiveValue();
			BVariant variantValue;
			value->ToVariant(variantValue);
			if (variantValue.Type() == B_STRING_TYPE)
				errorMessage.SetTo(variantValue.ToString());
			else
				address = variantValue.ToUInt64();
		}
	} else
		errorMessage = strerror(fExpressionResult);

	if (!errorMessage.IsEmpty()) {
		printf("Unable to evaluate expression: %s\n",
			errorMessage.String());
		return B_ERROR;
	}

	return B_OK;
}


status_t
CliContext::GetMemoryBlock(target_addr_t address, TeamMemoryBlock*& block)
{
	AutoLocker<BLocker> locker(fLock);
	if (fCurrentBlock == NULL || !fCurrentBlock->Contains(address)) {
		GetUserInterfaceListener()->InspectRequested(address, this);
		_WaitForEvent(MSG_TEAM_MEMORY_BLOCK_RETRIEVED);
		if (fTerminating)
			return B_INTERRUPTED;
	}

	block = fCurrentBlock;
	return B_OK;
}


const char*
CliContext::PromptUser(const char* prompt)
{
	fPrompt = prompt;

	int count;
	const char* line = el_gets(fEditLine, &count);

	fPrompt = NULL;

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
	fListener->UserInterfaceQuitRequested(
		killTeam
			? UserInterfaceListener::QUIT_OPTION_ASK_KILL_TEAM
			: UserInterfaceListener::QUIT_OPTION_ASK_RESUME_TEAM);

	WaitForEvent(MSG_QUIT);
}


void
CliContext::WaitForThreadOrUser()
{
// TODO: Deal with SIGINT as well!

	AutoLocker<BLocker> locker(fLock);

	while (fStoppedThread == NULL)
		_WaitForEvent(MSG_THREAD_STATE_CHANGED);

	if (fCurrentThread == NULL)
		SetCurrentThread(fStoppedThread);
}


void
CliContext::WaitForEvent(uint32 event) {
	AutoLocker<BLocker> locker(fLock);
	_WaitForEvent(event);
}


void
CliContext::MessageReceived(BMessage* message)
{
	fLock.Lock();

	int32 threadID;
	message->FindInt32("thread", &threadID);

	const char* threadName;
	message->FindString("threadName", &threadName);

	switch (message->what) {
		case MSG_THREAD_ADDED:
			printf("[new thread: %" B_PRId32 " \"%s\"]\n", threadID,
				threadName);
			break;
		case MSG_THREAD_REMOVED:
			printf("[thread terminated: %" B_PRId32 " \"%s\"]\n",
				threadID, threadName);
			break;
		case MSG_THREAD_STATE_CHANGED:
		{
			AutoLocker< ::Team> locker(fTeam);
			::Thread* thread = fTeam->ThreadByID(threadID);

			if (thread->State() == THREAD_STATE_STOPPED) {
				printf("[thread stopped: %" B_PRId32 " \"%s\"]\n",
					threadID, threadName);
				fStoppedThread.SetTo(thread);
			} else {
				fStoppedThread = NULL;
			}
			break;
		}
		case MSG_THREAD_STACK_TRACE_CHANGED:
			if (threadID == fCurrentThread->ID()) {
				AutoLocker< ::Team> locker(fTeam);
				::Thread* thread = fTeam->ThreadByID(threadID);

				fCurrentStackTrace = thread->GetStackTrace();
				fCurrentStackTrace->AcquireReference();
				SetCurrentStackFrameIndex(0);
			}
			break;
		case MSG_TEAM_MEMORY_BLOCK_RETRIEVED:
		{
			TeamMemoryBlock* block = NULL;
			if (message->FindPointer("block",
					reinterpret_cast<void **>(&block)) != B_OK) {
				break;
			}

			if (fCurrentBlock != NULL) {
				fCurrentBlock->ReleaseReference();
			}

			// reference acquired in MemoryBlockRetrieved
			fCurrentBlock = block;
			break;
		}
		case MSG_EXPRESSION_EVALUATED:
		{
			status_t result;
			if (message->FindInt32("result", &result) != B_OK) {
				break;
			}

			fExpressionResult = result;

			ExpressionResult* value = NULL;
			message->FindPointer("value", reinterpret_cast<void**>(&value));

			if (fExpressionValue != NULL) {
				fExpressionValue->ReleaseReference();
			}

			// reference acquired in ExpressionEvaluated
			fExpressionValue = value;
			break;
		}
		default:
			BLooper::MessageReceived(message);
			break;
	}

	fEventOccurred = message->what;

	fLock.Unlock();

	release_sem(fWaitForEventSemaphore);
	// all of the code that was waiting on the semaphore runs
	acquire_sem(fWaitForEventSemaphore);

	fLock.Lock();
	fEventOccurred = 0;
	fLock.Unlock();
}


void
CliContext::ThreadAdded(const Team::ThreadEvent& threadEvent)
{
	BMessage message(MSG_THREAD_ADDED);
	message.AddInt32("thread", threadEvent.GetThread()->ID());
	message.AddString("threadName", threadEvent.GetThread()->Name());
	PostMessage(&message);
}


void
CliContext::ThreadRemoved(const Team::ThreadEvent& threadEvent)
{
	BMessage message(MSG_THREAD_REMOVED);
	message.AddInt32("thread", threadEvent.GetThread()->ID());
	message.AddString("threadName", threadEvent.GetThread()->Name());
	PostMessage(&message);
}


void
CliContext::ThreadStateChanged(const Team::ThreadEvent& threadEvent)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	message.AddInt32("thread", threadEvent.GetThread()->ID());
	message.AddString("threadName", threadEvent.GetThread()->Name());
	PostMessage(&message);
}


void
CliContext::ThreadStackTraceChanged(const Team::ThreadEvent& threadEvent)
{
	if (threadEvent.GetThread()->State() != THREAD_STATE_STOPPED)
		return;

	BMessage message(MSG_THREAD_STACK_TRACE_CHANGED);
	message.AddInt32("thread", threadEvent.GetThread()->ID());
	message.AddString("threadName", threadEvent.GetThread()->Name());
	PostMessage(&message);
}


void
CliContext::ExpressionEvaluated(ExpressionInfo* info, status_t result,
	ExpressionResult* value)
{
	BMessage message(MSG_EXPRESSION_EVALUATED);
	message.AddInt32("result", result);

	if (value != NULL) {
		value->AcquireReference();
		message.AddPointer("value", value);
	}

	PostMessage(&message);
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
}


void
CliContext::CoreFileChanged(const Team::CoreFileChangedEvent& event)
{
	printf("Successfully saved core file to %s\n",
		event.GetTargetPath());
}


void
CliContext::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
	if (block != NULL)
		block->AcquireReference();

	BMessage message(MSG_TEAM_MEMORY_BLOCK_RETRIEVED);
	message.AddPointer("block", block);
	PostMessage(&message);
}


void
CliContext::ValueNodeChanged(ValueNodeChild* nodeChild, ValueNode* oldNode,
	ValueNode* newNode)
{
	BMessage message(MSG_VALUE_NODE_CHANGED);
	PostMessage(&message);
}


void
CliContext::ValueNodeChildrenCreated(ValueNode* node)
{
	BMessage message(MSG_VALUE_NODE_CHANGED);
	PostMessage(&message);
}


void
CliContext::ValueNodeChildrenDeleted(ValueNode* node)
{
	BMessage message(MSG_VALUE_NODE_CHANGED);
	PostMessage(&message);
}


void
CliContext::ValueNodeValueChanged(ValueNode* oldNode)
{
	BMessage message(MSG_VALUE_NODE_CHANGED);
	PostMessage(&message);
}


/*static*/ const char*
CliContext::_GetPrompt(EditLine* editLine)
{
	return sCurrentContext != NULL ? sCurrentContext->fPrompt : NULL;
}


void
CliContext::_WaitForEvent(uint32 event) {
	if (fTerminating)
		return;

	do {
		fLock.Unlock();
		while (acquire_sem(fWaitForEventSemaphore) == B_INTERRUPTED) {
		}
		fLock.Lock();
		release_sem(fWaitForEventSemaphore);
	} while (fEventOccurred != event && !fTerminating);
}

