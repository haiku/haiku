/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIZEOF_TEST_H
#define SIZEOF_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class SizeofTest: public BTestCase {
public:
					SizeofTest();
	virtual			~SizeofTest();

			void	TestSize();

	static	void	AddTests(BTestSuite& suite);
};


#endif


