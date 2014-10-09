/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATE_FORMAT_TEST_H
#define DATE_FORMAT_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class DateFormatTest: public BTestCase {
public:
					DateFormatTest();
	virtual			~DateFormatTest();

			void	TestCustomFormat();
			void	TestFormat();
			void	TestFormatDate();
			void	TestMonthNames();
			void	TestParseDate();
			void	TestParseTime();

	static	void	AddTests(BTestSuite& suite);
};


#endif
