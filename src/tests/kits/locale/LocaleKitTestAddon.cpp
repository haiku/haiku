/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "UnicodeCharTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("LocaleKit");

	UnicodeCharTest::AddTests(*suite);

	return suite;
}
