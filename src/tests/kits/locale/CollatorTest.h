/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef COLLATOR_TEST_H
#define COLLATOR_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class CollatorTest: public BTestCase {
public:
					CollatorTest();
	virtual			~CollatorTest();

			void	TestSortKeys();

	static	void	AddTests(BTestSuite& suite);
};


#endif
