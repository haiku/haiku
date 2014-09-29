/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_TEST_H
#define LANGUAGE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class LanguageTest: public BTestCase {
public:
					LanguageTest();
	virtual			~LanguageTest();

			void	TestLanguage();

	static	void	AddTests(BTestSuite& suite);
};


#endif
