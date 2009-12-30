/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TestVisitor.h"


TestVisitor::~TestVisitor()
{
}


bool
TestVisitor::VisitTest(Test* test)
{
	return false;
}


bool
TestVisitor::VisitTestSuitePre(TestSuite* suite)
{
	return false;
}


bool
TestVisitor::VisitTestSuitePost(TestSuite* suite)
{
	return false;
}
