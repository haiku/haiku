/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Test.h"

#include "TestVisitor.h"


// #pragma mark - Test


Test::Test(const char* name)
	:
	fName(name),
	fSuite(NULL)
{
}


Test::~Test()
{
}


void
Test::SetSuite(TestSuite* suite)
{
	fSuite = suite;
}


bool
Test::IsLeafTest() const
{
	return true;
}


status_t
Test::Setup(TestContext& context)
{
	return B_OK;
}


bool
Test::Run(TestContext& context, const char* name)
{
// TODO: Report error!
	return false;
}


void
Test::Cleanup(TestContext& context, bool setupOK)
{
}


Test*
Test::Visit(TestVisitor& visitor)
{
	return visitor.VisitTest(this) ? this : NULL;
}


// #pragma mark - StandardTestDelegate


StandardTestDelegate::StandardTestDelegate()
{
}


StandardTestDelegate::~StandardTestDelegate()
{
}


status_t
StandardTestDelegate::Setup(TestContext& context)
{
	return B_OK;
}


void
StandardTestDelegate::Cleanup(TestContext& context, bool setupOK)
{
}
