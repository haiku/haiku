// BMPTranslatorTest.cpp
#include "BMPTranslatorTest.h"
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

// Suite
CppUnit::Test *
BMPTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<BMPTranslatorTest> TC;
		
	suite->addTest(
		new TC("BMPTranslator DummyTest",
			&BMPTranslatorTest::DummyTest));
#if !TEST_R5
	suite->addTest(
		new TC("BMPTranslator BTranslatorTest",
			&BMPTranslatorTest::BTranslatorTest));
#endif
		
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

// BTranslatorTest
#if !TEST_R5
void
BMPTranslatorTest::BTranslatorTest()
{
	// . Make sure the add_on loads
	NextSubTest();
	const char *path = "/boot/home/config/add-ons/Translators/BMPTranslator";
	image_id image = load_add_on(path);
	CPPUNIT_ASSERT(image >= 0);
	
	// . Load in function to make the object
	NextSubTest();
	BTranslator *(*pMakeNthTranslator)(int32 n,image_id you,uint32 flags,...);
	status_t err = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void **)&pMakeNthTranslator);
	CPPUNIT_ASSERT(!err);

	// . Make sure the function returns a pointer to a BTranslator
	NextSubTest();
	BTranslator *ptran = pMakeNthTranslator(0, image, 0);
	CPPUNIT_ASSERT(ptran);
	
	// . Make sure the function only returns one BTranslator
	NextSubTest();
	CPPUNIT_ASSERT(!pMakeNthTranslator(1, image, 0));
	
	// . The translator should only have one reference
	NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
	// . Make sure Acquire returns a BTranslator even though its
	// already been Acquired once
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);
	
	// . Acquired twice, refcount should be 2
	NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);
	
	// . Release should return ptran because it is still acquired
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == ptran);
	
	// . A name would be nice
	NextSubTest();
	const char *tranname = ptran->TranslatorName();
	CPPUNIT_ASSERT(tranname);
	printf(" {%s} ", tranname);
	
	// . More info would be nice
	NextSubTest();
	const char *traninfo = ptran->TranslatorInfo();
	CPPUNIT_ASSERT(traninfo);
	printf(" {%s} ", traninfo);
	
	// . What version are you?
	// (when ver == 100, that means that version is 1.00)
	NextSubTest();
	int32 ver = ptran->TranslatorVersion();
	CPPUNIT_ASSERT((ver / 100) > 0);
	printf(" {%d} ", (int) ver);
	
	// . Input formats?
	NextSubTest();
	{
		int32 incount = 0;
		const translation_format *pins = ptran->InputFormats(&incount);
		CPPUNIT_ASSERT(incount == 2);
		CPPUNIT_ASSERT(pins);
		// . must support BMP and bits formats
		for (int32 i = 0; i < incount; i++) {
			CPPUNIT_ASSERT(pins[i].group == B_TRANSLATOR_BITMAP);
			CPPUNIT_ASSERT(pins[i].quality > 0 && pins[i].quality <= 1);
			CPPUNIT_ASSERT(pins[i].capability > 0 && pins[i].capability <= 1);
			CPPUNIT_ASSERT(pins[i].MIME);
			CPPUNIT_ASSERT(pins[i].name);

			if (pins[i].type == B_TRANSLATOR_BITMAP) {
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, "image/x-be-bitmap") == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name,
					"Be Bitmap Format (BMPTranslator)") == 0);
			} else if (pins[i].type == B_BMP_FORMAT) {
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, "image/x-bmp") == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name, "BMP image") == 0);
			} else
				CPPUNIT_ASSERT(false);
		}
	}
	
	// . Output formats?
	NextSubTest();
	{
		int32 outcount = 0;
		const translation_format *pouts = ptran->OutputFormats(&outcount);
		CPPUNIT_ASSERT(outcount == 2);
		CPPUNIT_ASSERT(pouts);
		// . must support BMP and bits formats
		for (int32 i = 0; i < outcount; i++) {
			CPPUNIT_ASSERT(pouts[i].group == B_TRANSLATOR_BITMAP);
			CPPUNIT_ASSERT(pouts[i].quality > 0 && pouts[i].quality <= 1);
			CPPUNIT_ASSERT(pouts[i].capability > 0 && pouts[i].capability <= 1);
			CPPUNIT_ASSERT(pouts[i].MIME);
			CPPUNIT_ASSERT(pouts[i].name);
	
			if (pouts[i].type == B_TRANSLATOR_BITMAP) {
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, "image/x-be-bitmap") == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name,
					"Be Bitmap Format (BMPTranslator)") == 0);
			} else if (pouts[i].type == B_BMP_FORMAT) {
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, "image/x-bmp") == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name, "BMP image (MS format)") == 0);
			} else
				CPPUNIT_ASSERT(false);
		}
	}
	
	// . MakeConfigurationView
	// Needs a BApplication object to work
	/*
	NextSubTest();
	{
		BMessage *pmsg = NULL;
		BView *pview = NULL;
		BRect dummy;
		status_t result = ptran->MakeConfigurationView(pmsg, &pview, &dummy);
		CPPUNIT_ASSERT(result == B_OK);
		printf(" monkey ");
		CPPUNIT_ASSERT(pview);
		printf(" bear ");
	}
	*/
	
	// . Release should return NULL because Release has been called
	// as many times as it has been acquired
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
	
	// . Unload Add-on
	NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK); 
}
#endif

// DummyTest
void
BMPTranslatorTest::DummyTest()
{
	// 0. Tautology
	NextSubTest();
	{
		printf("Hello from mars.");
		CPPUNIT_ASSERT( true == true );
	}
}
