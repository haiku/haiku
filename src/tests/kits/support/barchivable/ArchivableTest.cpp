/*
	$Id:
*/

#include "BArchivableTester.h"
#include "ValidateInstantiationTester.h"
#include "InstantiateObjectTester.h"
#include "FindInstantiationFuncTester.h"
#include "ArchivableTest.h"
#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"

CppUnit::Test* ArchivableTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(TBArchivableTestCase::Suite());
	testSuite->addTest(TValidateInstantiationTest::Suite());
	testSuite->addTest(TInstantiateObjectTester::Suite());
	testSuite->addTest(TFindInstantiationFuncTester::Suite());
	
	return testSuite;
}


