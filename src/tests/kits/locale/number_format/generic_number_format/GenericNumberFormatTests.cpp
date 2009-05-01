#include <cppunit/TestSuite.h>

#include "GenericNumberFormatTests.h"

#include "GenericNumberFormatConstructorTest.h"

// getTestSuite
CppUnit::Test*
GenericNumberFormatTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();

	testSuite->addTest(GenericNumberFormatConstructorTest::Suite());

	return testSuite;
}
