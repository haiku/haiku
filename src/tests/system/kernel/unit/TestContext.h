/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_CONTEXT_H
#define TEST_CONTEXT_H


#include <stdarg.h>
#include <stdio.h>

#include <KernelExport.h>

#include "TestOutput.h"


class Test;
class TestContext;
class TestError;


struct TestOptions {
			bool				panicOnFailure;
			bool				quitAfterFailure;

	public:
								TestOptions();
};


struct GlobalTestContext {
								GlobalTestContext(TestOutput& output,
									TestOptions& options);
								~GlobalTestContext();

	static	GlobalTestContext*	Current();

			TestOutput&			Output() const	{ return fOutput; }
			TestOptions&		Options() const	{ return fOptions; }

			TestContext*		CurrentContext() const
									{ return fCurrentContext; }
			void				SetCurrentContext(TestContext* context);

			int32				TotalTests() const	{ return fTotalTests; }
			int32				FailedTests() const	{ return fFailedTests; }

			void				TestDone(bool success);

			thread_id			SpawnThread(thread_func function,
									const char* name, int32 priority,
									void* arg);

private:
			struct ThreadEntry {
				ThreadEntry*		globalNext;
				ThreadEntry*		contextNext;
				GlobalTestContext*	context;
				thread_id			thread;

				ThreadEntry(GlobalTestContext* context)
					:
					globalNext(NULL),
					contextNext(NULL),
					context(context),
					thread(find_thread(NULL))
				{
				}
			};

			struct ThreadCookie;

private:
	static	void				_SetCurrent(ThreadEntry* entry);
	static	void				_UnsetCurrent(ThreadEntry* entry);
	static	status_t			_ThreadEntry(void* data);

private:
	static	ThreadEntry*		sGlobalThreads;
			ThreadEntry*		fThreads;
			ThreadEntry			fThreadEntry;
			TestOutput&			fOutput;
			TestOptions&		fOptions;
			TestContext*		fCurrentContext;
			int32				fTotalTests;
			int32				fFailedTests;
};


class TestContext {
public:
								TestContext(GlobalTestContext* globalContext);
								TestContext(TestContext& parent, Test* test);
								~TestContext();

	static	TestContext*		Current();

			GlobalTestContext*	GlobalContext() const { return fGlobalContext; }
			TestContext*		Parent() const	{ return fParent; }
			int32				Level() const	{ return fLevel; }
			TestOutput&			Output() const
									{ return fGlobalContext->Output(); }
			TestOptions&		Options() const
									{ return fGlobalContext->Options(); }

	inline	int					PrintArgs(const char* format, va_list args);
	inline	int					Print(const char* format,...);

			void				ErrorArgs(const char* format, va_list args);
	inline	void				Error(const char* format,...);

	inline	void				AssertFailed(const char* file, int line,
									const char* function,
									const char* condition);
	inline	void				AssertFailed(const char* file, int line,
									const char* function,
									const char* condition,
									const char* format,...);

			void				TestDone(bool success);

private:
			GlobalTestContext*	fGlobalContext;
			TestContext*		fParent;
			Test*				fTest;
			TestError*			fErrors;
			int32				fLevel;
};


/*static*/ inline TestContext*
TestContext::Current()
{
	return GlobalTestContext::Current()->CurrentContext();
}


int
TestContext::PrintArgs(const char* format, va_list args)
{
	return Output().PrintArgs(format, args);
}


int
TestContext::Print(const char* format,...)
{
	va_list args;
	va_start(args, format);
	int result = Output().PrintArgs(format, args);
	va_end(args);

	return result;
}


void
TestContext::Error(const char* format,...)
{
	va_list args;
	va_start(args, format);
	ErrorArgs(format, args);
	va_end(args);
}


void
TestContext::AssertFailed(const char* file, int line, const char* function,
	const char* condition)
{
	Error("ASSERT FAILED at %s:%d %s: %s\n", file, line, function, condition);
}


void
TestContext::AssertFailed(const char* file, int line, const char* function,
	const char* condition, const char* format,...)
{
	char buffer[256];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	Error("ASSERT FAILED at %s:%d %s: %s; %s\n", file, line, function,
		condition, buffer);
}


#endif	// TEST_CONTEXT_H
