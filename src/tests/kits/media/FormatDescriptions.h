/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef FORMAT_DESCRIPTIONS_TEST_H
#define FORMAT_DESCRIPTIONS_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class FormatDescriptionsTest: public BTestCase {
public:
					FormatDescriptionsTest();
	virtual			~FormatDescriptionsTest();

			void	TestCompare();

	static	void	AddTests(BTestSuite& suite);
};


#endif


