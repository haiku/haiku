//------------------------------------------------------------------------------
//	TargetTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/kernel/OS.h>

#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "Helpers.h"
#include "TargetTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
 */
void TargetTester::Test1()
{
}


Test* TargetTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST(SuiteOfTests, TargetTester, Test1);

	return SuiteOfTests;
}

