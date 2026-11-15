/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "bmessage/MessageTest.h"
#include "bmessagequeue/MessageQueueTest.h"
#include "bmessagerunner/MessageRunnerTest.h"
#include "bmessenger/MessengerTest.h"
#include "bpropertyinfo/PropertyInfoTest.h"
#include "broster/RosterTest.h"
#include "RegistrarThreadManagerTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("App");

	// ##### Add test suites here #####
//	suite->addTest("BMessage", MessageTestSuite());
	suite->addTest("BMessageQueue", MessageQueueTestSuite());
	suite->addTest("BMessageRunner", MessageRunnerTestSuite());
	suite->addTest("BMessenger", MessengerTestSuite());
	suite->addTest("BPropertyInfo", PropertyInfoTestSuite());
//	suite->addTest("BRoster", RosterTestSuite());
	// TODO: calls Lock on destruction, hangs
	//suite->addTest("RegistrarThreadManager", RegistrarThreadManagerTest::Suite());
	
	return suite;
}

const char*
getTestSuiteName()
{
	return "AppKit";
}
