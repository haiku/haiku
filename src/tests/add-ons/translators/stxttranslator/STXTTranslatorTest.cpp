// STXTTranslatorTest.cpp
#include "STXTTranslatorTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <image.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <Message.h>
#include <View.h>
#include <Rect.h>
#include <File.h>
#include <DataIO.h>
//#include "../../../../add-ons/translators/bmptranslator/STXTTranslator.h"

// Suite
CppUnit::Test *
STXTTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<STXTTranslatorTest> TC;
		
	suite->addTest(
		new TC("STXTTranslator DummyTest",
			&STXTTranslatorTest::DummyTest));
#if !TEST_R5

#endif
		
	return suite;
}		

// setUp
void
STXTTranslatorTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
STXTTranslatorTest::tearDown()
{
	BTestCase::tearDown();
}

#if !TEST_R5

#endif // #if !TEST_R5

// DummyTest
void
STXTTranslatorTest::DummyTest()
{
	// 0. Tautology
	NextSubTest();
	printf("Hello from mars.");
	CPPUNIT_ASSERT( true == true );
}
