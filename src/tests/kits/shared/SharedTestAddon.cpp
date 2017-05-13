/*
 * Copyright 2017, Andrew Lindesay, apl@lindesay.co.nz
 * Copyright 2012-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CalendarViewTest.h"
#include "DriverSettingsMessageAdapterTest.h"
#include "GeolocationTest.h"
#include "NaturalCompareTest.h"
#include "JsonEndToEndTest.h"
#include "JsonErrorHandlingTest.h"
#include "JsonTextWriterTest.h"
#include "JsonToMessageTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("Shared");

	CalendarViewTest::AddTests(*suite);
	DriverSettingsMessageAdapterTest::AddTests(*suite);
	GeolocationTest::AddTests(*suite);
	NaturalCompareTest::AddTests(*suite);
	JsonEndToEndTest::AddTests(*suite);
	JsonErrorHandlingTest::AddTests(*suite);
	JsonTextWriterTest::AddTests(*suite);
	JsonToMessageTest::AddTests(*suite);

	return suite;
}
