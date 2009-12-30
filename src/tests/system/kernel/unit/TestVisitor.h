/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_VISITOR_H
#define TEST_VISITOR_H


class Test;
class TestSuite;


class TestVisitor {
public:
	virtual						~TestVisitor();

	virtual	bool				VisitTest(Test* test);

	virtual	bool				VisitTestSuitePre(TestSuite* suite);
	virtual	bool				VisitTestSuitePost(TestSuite* suite);
};


#endif	// TEST_VISITOR_H
