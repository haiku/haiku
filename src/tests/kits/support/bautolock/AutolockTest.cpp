/*
	$Id: 
*/
	
#include "AutolockLockerTest.h"
#include "AutolockLooperTest.h"
#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"

CppUnit::Test* AutolockTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(AutolockLockerTest::suite());
	testSuite->addTest(AutolockLooperTest::suite());
	
	return testSuite;
}

