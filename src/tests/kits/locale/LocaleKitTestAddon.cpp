/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CollatorTest.h"
#include "DateFormatTest.h"
#include "DurationFormatTest.h"
#include "LanguageTest.h"
#include "NumberFormatTest.h"
#include "RelativeDateTimeFormatTest.h"
#include "StringFormatTest.h"
#include "UnicodeCharTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("LocaleKit");

	CollatorTest::AddTests(*suite);
	DateFormatTest::AddTests(*suite);
	DurationFormatTest::AddTests(*suite);
	LanguageTest::AddTests(*suite);
	NumberFormatTest::AddTests(*suite);
	StringFormatTest::AddTests(*suite);
	RelativeDateTimeFormatTest::AddTests(*suite);
	UnicodeCharTest::AddTests(*suite);

	return suite;
}
