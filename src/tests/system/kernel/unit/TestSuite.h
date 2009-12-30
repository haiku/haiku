/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_SUITE_H
#define TEST_SUITE_H


#include <new>

#include "Test.h"


class TestSuite : public Test {
public:
								TestSuite(const char* name);
	virtual						~TestSuite();

			int32				CountTests() const;
			Test*				TestAt(int32 index) const;
			Test*				FindTest(const char* name,
									int32 nameLength = -1) const;

			bool				AddTest(Test* test);

	virtual	bool				IsLeafTest() const;

	virtual	bool				Run(TestContext& context);
	virtual	bool				Run(TestContext& context, const char* name);

	virtual	Test*				Visit(TestVisitor& visitor);

private:
			bool				_Run(TestContext& context, Test* test,
									const char* name);

private:
			Test**				fTests;
			int32				fTestCount;
};


#define ADD_TEST(suite, test)			\
	do {								\
		if (!suite->AddTest(test)) {	\
			delete test;				\
			delete suite;				\
			return NULL;				\
		}								\
	} while (false)

#define ADD_STANDARD_TEST(suite, type, method)								\
	do {																	\
		type* object = new(std::nothrow) type;								\
		if (object == NULL)													\
			return NULL;													\
																			\
		StandardTest<type>* test = new(std::nothrow) StandardTest<type>(	\
			#method, object, &type::method);								\
		if (test == NULL) {													\
			delete object;													\
			return NULL;													\
		}																	\
		ADD_TEST(suite, test);												\
	} while (false)


#endif	// TEST_SUITE_H
