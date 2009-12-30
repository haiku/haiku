/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TestManager.h"

#include <string.h>

#include "TestOutput.h"
#include "TestVisitor.h"


TestManager::TestManager()
	:
	TestSuite("all")
{
}


TestManager::~TestManager()
{
}


void
TestManager::ListTests(TestOutput& output)
{
	struct Visitor : TestVisitor {
		Visitor(TestOutput& output)
			:
			fOutput(output),
			fLevel(0)
		{
		}

		virtual bool VisitTest(Test* test)
		{
			fOutput.Print("%*s%s\n", fLevel * 2, "", test->Name());
			return false;
		}

		virtual bool VisitTestSuitePre(TestSuite* suite)
		{
			if (fLevel > 0)
				VisitTest(suite);
			fLevel++;
			return false;
		}

		virtual bool VisitTestSuitePost(TestSuite* suite)
		{
			fLevel--;
			return false;
		}

	private:
		TestOutput&	fOutput;
		int			fLevel;
	} visitor(output);

	output.Print("Available tests:\n");
	Visit(visitor);
}


void
TestManager::RunTests(GlobalTestContext& globalContext,
	const char* const* tests, int testCount)
{
	TestContext context(&globalContext);

	context.Print("Running tests:\n");

	if (testCount == 0 || (testCount == 1 && strcmp(tests[0], "all") == 0)) {
		Run(context);
	} else {
		for (int i = 0; i < testCount; i++) {
			bool result = Run(context, tests[i]);
			if (!result && context.Options().quitAfterFailure)
				break;
		}
	}

	context.Print("run tests: %ld, failed tests: %ld\n",
		globalContext.TotalTests(), globalContext.FailedTests());
}
