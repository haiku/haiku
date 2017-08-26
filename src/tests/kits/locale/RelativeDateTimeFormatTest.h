/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Akshay Agarwal <agarwal.akshay.akshay8@gmail.com>
 */
#ifndef RELATIVE_DATE_TIME_FORMAT_TEST_H
#define RELATIVE_DATE_TIME_FORMAT_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class RelativeDateTimeFormatTest: public BTestCase {
public:
					RelativeDateTimeFormatTest();
	virtual			~RelativeDateTimeFormatTest();

			void	TestDefault();
			void	TestFormat();

	static	void	AddTests(BTestSuite& suite);
};


#endif
