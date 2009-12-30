/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TestSuite.h"

#include <new>

#include <string.h>

#include "TestVisitor.h"


TestSuite::TestSuite(const char* name)
	:
	Test(name),
	fTests(NULL),
	fTestCount(0)
{
}


TestSuite::~TestSuite()
{
	for (int32 i = 0; i < fTestCount; i++)
		delete fTests[i];
	delete[] fTests;
}


int32
TestSuite::CountTests() const
{
	return fTestCount;
}


Test*
TestSuite::TestAt(int32 index) const
{
	return index >= 0 && index < fTestCount ? fTests[index] : NULL;
}


Test*
TestSuite::FindTest(const char* name, int32 nameLength) const
{
	if (nameLength < 0)
		nameLength = strlen(name);

	for (int32 i = 0; Test* test = TestAt(i); i++) {
		if (strlen(test->Name()) == (size_t)nameLength
			&& strncmp(test->Name(), name, nameLength) == 0) {
			return test;
		}
	}

	return NULL;
}


bool
TestSuite::AddTest(Test* test)
{
	if (test == NULL)
		return test;

	Test** tests = new(std::nothrow) Test*[fTestCount + 1];
	if (tests == NULL) {
		delete test;
		return false;
	}

	if (fTestCount > 0)
		memcpy(tests, fTests, sizeof(Test*) * fTestCount);

	delete[] fTests;

	fTests = tests;
	fTests[fTestCount++] = test;

	test->SetSuite(this);

	return true;
}


bool
TestSuite::IsLeafTest() const
{
	return false;
}


bool
TestSuite::Run(TestContext& context)
{
	for (int32 i = 0; Test* test = TestAt(i); i++) {
		bool result = _Run(context, test, NULL);
		if (!result && context.Options().quitAfterFailure)
			return false;
	}

	return true;
}


bool
TestSuite::Run(TestContext& context, const char* name)
{
	const char* separator = strstr(name, "::");
	Test* test = FindTest(name, separator != NULL ? separator - name : -1);
	if (test == NULL) {
		context.Print("No such test: \"%.*s\"\n",
			int(separator != NULL ? separator - name : strlen(name)), name);
		return !context.Options().quitAfterFailure;
	}

	return _Run(context, test, separator != NULL ? separator + 2 : NULL)
		|| !context.Options().quitAfterFailure;
}


Test*
TestSuite::Visit(TestVisitor& visitor)
{
	if (visitor.VisitTestSuitePre(this))
		return this;

	for (int32 i = 0; Test* test = TestAt(i); i++) {
		if (Test* foundTest = test->Visit(visitor))
			return foundTest;
	}

	return visitor.VisitTestSuitePost(this) ? this : NULL;
}


bool
TestSuite::_Run(TestContext& context, Test* test, const char* name)
{
	TestContext subContext(context, test);

	status_t error = test->Setup(subContext);
	if (error != B_OK) {
		subContext.Error("setup failed\n");
		test->Cleanup(subContext, false);
		subContext.TestDone(false);
		return false;
	}

	bool result = name != NULL
		? test->Run(subContext, name) : test->Run(subContext);
	test->Cleanup(subContext, true);

	subContext.TestDone(result);

	return result;
}
