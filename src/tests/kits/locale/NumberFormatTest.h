/*
 * Copyright 2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef NUMBER_FORMAT_TEST_H
#define NUMBER_FORMAT_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class NumberFormatTest: public BTestCase {
public:
					NumberFormatTest();
	virtual			~NumberFormatTest();

			void	TestPercentTurkish();
			void	TestPercentEnglish();
			void	TestPercentGerman();

	static	void	AddTests(BTestSuite& suite);

private:
			void	_TestGeneralPercent(const char* languageCode,
						const char* expected);
};


#endif
