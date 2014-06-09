/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CookieTest.h"
#include "HttpTest.h"
#include "UrlTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("ServicesKit");

	CookieTest::AddTests(*suite);
	UrlTest::AddTests(*suite);
	HttpTest::AddTests(*suite);

	return suite;
}
