/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef URL_TEST_H
#define URL_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class UrlTest: public BTestCase {
public:
					UrlTest();
	virtual			~UrlTest();

			void	LengthTest();
			void	ExplodeImplodeTest();
			void	PathOnly();

	static	void	AddTests(BTestSuite& suite);
};


#endif
