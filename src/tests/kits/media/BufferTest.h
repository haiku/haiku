/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef BUFFER_TEST_H
#define BUFFER_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class BufferTest: public BTestCase {
public:
					BufferTest();
	virtual			~BufferTest();

			void	TestDefault();
			void	TestRef();
			void	TestSmall();

	static	void	AddTests(BTestSuite& suite);
};


#endif


