/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_FORMAT_TEST_H
#define MESSAGE_FORMAT_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class MessageFormatTest: public BTestCase {
public:
					MessageFormatTest();
	virtual			~MessageFormatTest();

			void	TestFormat();
			void	TestBogus();

	static	void	AddTests(BTestSuite& suite);
};


#endif
