/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H


#include "TestSuite.h"


class GlobalTestContext;
class TestOutput;


class TestManager : public TestSuite {
public:
								TestManager();
								~TestManager();

			void				ListTests(TestOutput& output);
			void				RunTests(GlobalTestContext& globalContext,
									const char* const* tests, int testCount);
};


#endif	// TEST_MANAGER_H
