/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_TEST_H
#define UTILITY_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class UtilityTest : public CppUnit::TestCase {
public:
								UtilityTest();
	virtual						~UtilityTest();

			void				TestTranslatePath();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// UTILITY_TEST_H
