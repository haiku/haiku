// BMPTranslatorTest.cpp
#include "BMPTranslatorTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

// Suite
CppUnit::Test *
BMPTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<BMPTranslatorTest> TC;
		
	suite->addTest( new TC("BMPTranslator DummyTest1", &BMPTranslatorTest::DummyTest) );
		
	return suite;
}		

// setUp
void
BMPTranslatorTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
BMPTranslatorTest::tearDown()
{
	BTestCase::tearDown();
}

// DummyTest
void
BMPTranslatorTest::DummyTest()
{
	// 1. Tautology
	NextSubTest();
	{
		printf("Hello from mars.\n");
		CPPUNIT_ASSERT( true == true );
	}
}
