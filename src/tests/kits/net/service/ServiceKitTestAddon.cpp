/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CookieTest.h"
#include "DataTest.h"
#include "GeolocationTest.h"
#include "HttpTest.h"
#include "UrlTest.h"
#include "FileTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("ServicesKit");

	CookieTest::AddTests(*suite);
	UrlTest::AddTests(*suite);
	HttpTest::AddTests(*suite);
	DataTest::AddTests(*suite);
	FileTest::AddTests(*suite);
	GeolocationTest::AddTests(*suite);

	return suite;
}
