// TIFFTranslatorTest.cpp
#include "TIFFTranslatorTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <image.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <Message.h>
#include <View.h>
#include <Rect.h>
#include <File.h>
#include <DataIO.h>
#include <Errors.h>
#include <OS.h>
#include "../../../../add-ons/translators/tifftranslator/TIFFTranslator.h"

// Suite
CppUnit::Test *
TIFFTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<TIFFTranslatorTest> TC;
			
	suite->addTest(
		new TC("TIFFTranslator IdentifyTest",
			&TIFFTranslatorTest::IdentifyTest));

	suite->addTest(
		new TC("TIFFTranslator TranslateTest",
			&TIFFTranslatorTest::TranslateTest));	

#if !TEST_R5
	suite->addTest(
		new TC("TIFFTranslator LoadAddOnTest",
			&TIFFTranslatorTest::LoadAddOnTest));
#endif
		
	return suite;
}		

// setUp
void
TIFFTranslatorTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
TIFFTranslatorTest::tearDown()
{
	BTestCase::tearDown();
}

void
TIFFTranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
}

void
TIFFTranslatorTest::TranslateTest()
{
	// Init
	NextSubTest();
}

#if !TEST_R5

void
TIFFTranslatorTest::LoadAddOnTest()
{
	// Make sure the add_on loads
	NextSubTest();
}

#endif // #if !TEST_R5
