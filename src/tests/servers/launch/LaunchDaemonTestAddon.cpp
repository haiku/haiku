/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "SettingsParserTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("LaunchDaemon");

	SettingsParserTest::AddTests(*suite);

	return suite;
}
