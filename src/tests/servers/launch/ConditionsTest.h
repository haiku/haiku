/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONDITIONS_TEST_H
#define CONDITIONS_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class Condition;


class ConditionsTest : public CppUnit::TestCase {
public:
								ConditionsTest();
	virtual						~ConditionsTest();

			void				TestEmpty();
			void				TestSafemode();
			void				TestFileExists();
			void				TestOr();
			void				TestAnd();
			void				TestNot();

	static	void				AddTests(BTestSuite& suite);

private:
			Condition*			_Condition(const char* string);
};


#endif	// CONDITIONS_TEST_H
