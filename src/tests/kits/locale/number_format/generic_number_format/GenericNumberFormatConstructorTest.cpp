#include <stdio.h>

#include <GenericNumberFormat.h>

#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "GenericNumberFormatConstructorTest.h"

using namespace CppUnit;

// Suite
Test *
GenericNumberFormatConstructorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<GenericNumberFormatConstructorTest> TC;

	suite->addTest(new TC("BGenericNumberFormat::Constructor Test1",
		&GenericNumberFormatConstructorTest::ConstructorTest1));

	return suite;
}

// ConstructorTest1
void GenericNumberFormatConstructorTest::ConstructorTest1()
{
	BGenericNumberFormat format;
}

