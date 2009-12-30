/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TestContext.h"

#include <util/AutoLock.h>

#include "Test.h"
#include "TestError.h"


static spinlock sLock = B_SPINLOCK_INITIALIZER;


// #pragma mark - TestOptions


TestOptions::TestOptions()
	:
	panicOnFailure(false),
	quitAfterFailure(false)
{
}


// #pragma mark - GlobalTestContext


struct GlobalTestContext::ThreadCookie {
	GlobalTestContext*	context;
	thread_func			function;
	void*				arg;

	ThreadCookie(GlobalTestContext* context, thread_func function, void* arg)
		:
		context(context),
		function(function),
		arg(arg)
	{
	}
};


/*static*/ GlobalTestContext::ThreadEntry* GlobalTestContext::sGlobalThreads
	= NULL;


GlobalTestContext::GlobalTestContext(TestOutput& output, TestOptions& options)
	:
	fThreads(NULL),
	fThreadEntry(this),
	fOutput(output),
	fOptions(options),
	fCurrentContext(NULL),
	fTotalTests(0),
	fFailedTests(0)
{
	_SetCurrent(&fThreadEntry);
}


GlobalTestContext::~GlobalTestContext()
{
	InterruptsSpinLocker locker(sLock);

	// remove all of our entries from the global list
	ThreadEntry** entry = &sGlobalThreads;
	while (*entry != NULL) {
		if ((*entry)->context == this)
			*entry = (*entry)->globalNext;
		else
			entry = &(*entry)->globalNext;
	}
}


/*static*/ GlobalTestContext*
GlobalTestContext::Current()
{
	thread_id thread = find_thread(NULL);

	InterruptsSpinLocker locker(sLock);

	ThreadEntry* entry = sGlobalThreads;
	while (entry != NULL) {
		if (entry->thread == thread)
			return entry->context;
		entry = entry->globalNext;
	}

	return NULL;
}


void
GlobalTestContext::SetCurrentContext(TestContext* context)
{
	fCurrentContext = context;
}


void
GlobalTestContext::TestDone(bool success)
{
	fTotalTests++;
	if (!success)
		fFailedTests++;
}


thread_id
GlobalTestContext::SpawnThread(thread_func function, const char* name,
	int32 priority, void* arg)
{
	ThreadCookie* cookie = new(std::nothrow) ThreadCookie(this, function, arg);
	if (cookie == NULL)
		return B_NO_MEMORY;

	thread_id thread = spawn_kernel_thread(_ThreadEntry, name, priority,
		cookie);
	if (thread < 0) {
		delete cookie;
		return thread;
	}

	return thread;
}


/*static*/ void
GlobalTestContext::_SetCurrent(ThreadEntry* entry)
{
	InterruptsSpinLocker locker(sLock);

	entry->contextNext = entry->context->fThreads;
	entry->context->fThreads = entry;

	entry->globalNext = sGlobalThreads;
	sGlobalThreads = entry;
}


/*static*/ void
GlobalTestContext::_UnsetCurrent(ThreadEntry* entryToRemove)
{
	InterruptsSpinLocker locker(sLock);

	// remove from the global list
	ThreadEntry** entry = &sGlobalThreads;
	while (*entry != NULL) {
		if (*entry == entryToRemove) {
			*entry = (*entry)->globalNext;
			break;
		}

		entry = &(*entry)->globalNext;
	}

	// remove from the context's list
	entry = &entryToRemove->context->fThreads;
	while (*entry != NULL) {
		if (*entry == entryToRemove) {
			*entry = (*entry)->contextNext;
			break;
		}

		entry = &(*entry)->contextNext;
	}
}


/*static*/ status_t
GlobalTestContext::_ThreadEntry(void* data)
{
	ThreadCookie* cookie = (ThreadCookie*)data;

	ThreadEntry entry(cookie->context);
	_SetCurrent(&entry);

	thread_func function = cookie->function;
	void* arg = cookie->arg;
	delete cookie;

	status_t result = function(arg);

	_UnsetCurrent(&entry);

	return result;
}


// #pragma mark - TestContext


TestContext::TestContext(GlobalTestContext* globalContext)
	:
	fGlobalContext(globalContext),
	fParent(NULL),
	fTest(NULL),
	fErrors(NULL),
	fLevel(0)
{
	fGlobalContext->SetCurrentContext(this);
}


TestContext::TestContext(TestContext& parent, Test* test)
	:
	fGlobalContext(parent.GlobalContext()),
	fParent(&parent),
	fTest(test),
	fErrors(NULL),
	fLevel(parent.Level() + 1)
{
	fGlobalContext->SetCurrentContext(this);

	if (fTest != NULL) {
		if (fTest->IsLeafTest())
			Print("%*s%s...", fLevel * 2, "", test->Name());
		else
			Print("%*s%s:\n", fLevel * 2, "", test->Name());
	}
}


TestContext::~TestContext()
{
	fGlobalContext->SetCurrentContext(fParent);

	while (fErrors != NULL) {
		TestError* error = fErrors;
		fErrors = error->ListLink();
		delete error;
	}
}


void
TestContext::TestDone(bool success)
{
	if (fTest != NULL && fTest->IsLeafTest()) {
		if (success) {
			Print(" ok\n");
		} else {
			Print(" FAILED\n");
			TestError* error = fErrors;
			while (error != NULL) {
				Print("%s", error->Message());
				error = error->ListLink();
			}
		}

		fGlobalContext->TestDone(success);
	}
}


void
TestContext::ErrorArgs(const char* format, va_list args)
{
	int size = vsnprintf(NULL, 0, format, args) + 1;
	char* buffer = (char*)malloc(size);
	if (buffer == NULL)
		return;

	vsnprintf(buffer, size, format, args);

	TestError* error = new(std::nothrow) TestError(fTest, buffer);
	if (error == NULL) {
		free(buffer);
		return;
	}

	InterruptsSpinLocker locker(sLock);
	error->ListLink() = fErrors;
	fErrors = error;

	if (Options().panicOnFailure)
		panic("Test check failed: %s", error->Message());
}
