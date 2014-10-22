/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef AREA_TEST_H
#define AREA_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class AreaTest: public BTestCase {
public:
					AreaTest();
	virtual			~AreaTest();

			void	TestAreas();

	static	void	AddTests(BTestSuite& suite);
};


#endif

