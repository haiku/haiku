/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CollatorTest.h"
#include "UnicodeCharTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("LocaleKit");

	CollatorTest::AddTests(*suite);
	UnicodeCharTest::AddTests(*suite);

	return suite;
}
