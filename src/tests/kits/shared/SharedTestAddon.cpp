/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CalendarViewTest.h"
#include "NaturalCompareTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("Shared");

	CalendarViewTest::AddTests(*suite);
	NaturalCompareTest::AddTests(*suite);

	return suite;
}
