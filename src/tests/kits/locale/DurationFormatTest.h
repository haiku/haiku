/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef DURATION_FORMAT_TEST_H
#define DURATION_FORMAT_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class DurationFormatTest: public BTestCase {
public:
					DurationFormatTest();
	virtual			~DurationFormatTest();

			void	TestDefault();
			void	TestDuration();
			void	TestTimeUnit();

	static	void	AddTests(BTestSuite& suite);
};


#endif
