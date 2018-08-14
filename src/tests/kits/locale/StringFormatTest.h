/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_FORMAT_TEST
#define STRING_FORMAT_TEST


#include <TestCase.h>
#include <TestSuite.h>


class StringFormatTest: public BTestCase {
public:
					StringFormatTest();
	virtual			~StringFormatTest();

			void	TestFormat();
			void	TestBogus();

	static	void	AddTests(BTestSuite& suite);
};


#endif
