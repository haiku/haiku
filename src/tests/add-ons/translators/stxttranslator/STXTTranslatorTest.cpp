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
#include <TranslatorRoster.h>
#include <Message.h>
#include <View.h>
#include <Rect.h>
#include <File.h>
#include <DataIO.h>
#include <Errors.h>
#include <OS.h>
#include "../../../../add-ons/translators/stxttranslator/STXTTranslator.h"

// Suite
CppUnit::Test *
STXTTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<STXTTranslatorTest> TC;
			
	suite->addTest(
		new TC("STXTTranslator IdentifyTest",
			&STXTTranslatorTest::IdentifyTest));

	suite->addTest(
		new TC("STXTTranslator TranslateTest",
			&STXTTranslatorTest::TranslateTest));	

#if !TEST_R5
	suite->addTest(
		new TC("STXTTranslator LoadAddOnTest",
			&STXTTranslatorTest::LoadAddOnTest));
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

void
CheckStyled(translator_info *pti)
{
	CPPUNIT_ASSERT(pti->type == B_STYLED_TEXT_FORMAT);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(pti->quality == 0.5);
	CPPUNIT_ASSERT(pti->capability == 0.5);
	CPPUNIT_ASSERT(strcmp(pti->name, "Be styled text file") == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, "text/x-vnd.Be-stxt") == 0);
}

void
CheckPlain(translator_info *pti)
{
	CPPUNIT_ASSERT(pti->type == B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(pti->quality > 0.39 && pti->quality < 0.41);
	CPPUNIT_ASSERT(pti->capability > 0.59 && pti->capability < 0.61);
	CPPUNIT_ASSERT(strcmp(pti->name, "Plain text file") == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, "text/plain") == 0);
}

void
IdentifyTests(STXTTranslatorTest *ptest, BTranslatorRoster *proster,
	const char **paths, int32 len, bool bplain)
{
	translator_info ti;
	printf(" [%d] ", (int) bplain);
	
	for (int32 i = 0; i < len; i++) {
		ptest->NextSubTest();
		BFile file;
		printf(" [%s] ", paths[i]);
		CPPUNIT_ASSERT(file.SetTo(paths[i], B_READ_ONLY) == B_OK);

		// Identify (output: B_TRANSLATOR_ANY_TYPE)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti) == B_OK);
		if (bplain)
			CheckPlain(&ti);
		else
			CheckStyled(&ti);
	
		// Identify (output: B_TRANSLATOR_TEXT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_TEXT) == B_OK);
		if (bplain)
			CheckPlain(&ti);
		else
			CheckStyled(&ti);
	
		// Identify (output: B_STYLED_TEXT_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_STYLED_TEXT_FORMAT) == B_OK);
		if (bplain)
			CheckPlain(&ti);
		else
			CheckStyled(&ti);
	}
}

void
STXTTranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/STXTTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
		
	// Identify (bad input, output types)
	NextSubTest();
	translator_info ti;
	memset(&ti, 0, sizeof(translator_info));
	result = proster->Identify(&wronginput, NULL, &ti, 0,
		NULL, B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
		
	// Identify (wrong type of input data)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	result = proster->Identify(&wronginput, NULL, &ti);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	// Identify (wrong magic)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	BFile wrongmagic("../src/tests/kits/translation/data/text/wrong_magic.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wrongmagic.InitCheck() == B_OK);
	result = proster->Identify(&wrongmagic, NULL, &ti);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	// Identify (wrong version)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	BFile wrongversion("../src/tests/kits/translation/data/text/wrong_version.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wrongversion.InitCheck() == B_OK);
	result = proster->Identify(&wrongversion, NULL, &ti);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	const char *aPlainFiles[] = {
		"../src/tests/kits/translation/data/text/ascii.txt",
		"../src/tests/kits/translation/data/text/japanese.txt",
		"../src/tests/kits/translation/data/text/multi_byte.txt",
		"../src/tests/kits/translation/data/text/one_length.txt",
		"../src/tests/kits/translation/data/text/sentence.txt",
		"../src/tests/kits/translation/data/text/symbols.txt",
		"../src/tests/kits/translation/data/text/zero_length.txt"
	};
	const char *aStyledFiles[] = {
		"../src/tests/kits/translation/data/text/ascii.stxt",
		"../src/tests/kits/translation/data/text/japanese.stxt",
		"../src/tests/kits/translation/data/text/multi_byte.stxt",
		"../src/tests/kits/translation/data/text/one_length.stxt",
		"../src/tests/kits/translation/data/text/sentence.stxt",
		"../src/tests/kits/translation/data/text/symbols.stxt",
		"../src/tests/kits/translation/data/text/zero_length.stxt",
		"../src/tests/kits/translation/data/text/zero_length_styl.stxt"
	};
	IdentifyTests(this, proster, aPlainFiles,
		sizeof(aPlainFiles) / sizeof(const char *), true);
	IdentifyTests(this, proster, aStyledFiles,
		sizeof(aStyledFiles) / sizeof(const char *), false);
}

void
STXTTranslatorTest::TranslateTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	off_t filesize = -1;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/STXTTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
	BFile output("/tmp/stxt_test.out", B_WRITE_ONLY | 
		B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(output.InitCheck() == B_OK);
	
	// Translate (bad input, output types)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
	
	// Translate (wrong type of input data)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
	
	// Translate (wrong type of input, B_TRANSLATOR_ANY_TYPE output)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
}

void
TestBTranslator(STXTTranslatorTest *ptest, BTranslator *ptran)
{	
	// . The translator should only have one reference
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
	// . Make sure Acquire returns a BTranslator even though its
	// already been Acquired once
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);
	
	// . Acquired twice, refcount should be 2
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);
	
	// . Release should return ptran because it is still acquired
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == ptran);
	
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);
	
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);
	
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == ptran);
	
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
	// . A name would be nice
	ptest->NextSubTest();
	const char *tranname = ptran->TranslatorName();
	CPPUNIT_ASSERT(tranname);
	printf(" {%s} ", tranname);
	
	// . More info would be nice
	ptest->NextSubTest();
	const char *traninfo = ptran->TranslatorInfo();
	CPPUNIT_ASSERT(traninfo);
	printf(" {%s} ", traninfo);
	
	// . What version are you?
	// (when ver == 100, that means that version is 1.00)
	ptest->NextSubTest();
	int32 ver = ptran->TranslatorVersion();
	CPPUNIT_ASSERT((ver / 100) > 0);
	printf(" {%d} ", (int) ver);
	
	// . Input formats?
	ptest->NextSubTest();
	{
		int32 incount = 0;
		const translation_format *pins = ptran->InputFormats(&incount);
		CPPUNIT_ASSERT(incount == 2);
		CPPUNIT_ASSERT(pins);
		// . must support STXT and TEXT formats
		for (int32 i = 0; i < incount; i++) {
			CPPUNIT_ASSERT(pins[i].group == B_TRANSLATOR_TEXT);
			CPPUNIT_ASSERT(pins[i].quality > 0 && pins[i].quality <= 1);
			CPPUNIT_ASSERT(pins[i].capability > 0 && pins[i].capability <= 1);
			CPPUNIT_ASSERT(pins[i].MIME);
			CPPUNIT_ASSERT(pins[i].name);

			if (pins[i].type == B_TRANSLATOR_TEXT) {
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, TEXT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name,
					"Plain text file") == 0);
			} else if (pins[i].type == B_STYLED_TEXT_FORMAT) {
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, STXT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name, "Be styled text file") == 0);
			} else
				CPPUNIT_ASSERT(false);
		}
	}
	
	// . Output formats?
	ptest->NextSubTest();
	{
		int32 outcount = 0;
		const translation_format *pouts = ptran->OutputFormats(&outcount);
		CPPUNIT_ASSERT(outcount == 2);
		CPPUNIT_ASSERT(pouts);
		// . must support STXT and TEXT formats
		for (int32 i = 0; i < outcount; i++) {
			CPPUNIT_ASSERT(pouts[i].group == B_TRANSLATOR_TEXT);
			CPPUNIT_ASSERT(pouts[i].quality > 0 && pouts[i].quality <= 1);
			CPPUNIT_ASSERT(pouts[i].capability > 0 && pouts[i].capability <= 1);
			CPPUNIT_ASSERT(pouts[i].MIME);
			CPPUNIT_ASSERT(pouts[i].name);
	
			if (pouts[i].type == B_TRANSLATOR_TEXT) {
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, TEXT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name,
					"Plain text file") == 0);
			} else if (pouts[i].type == B_STYLED_TEXT_FORMAT) {
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, STXT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name, "Be styled text file") == 0);
			} else
				CPPUNIT_ASSERT(false);
		}
	}
	
	// . Release should return NULL because Release has been called
	// as many times as it has been acquired
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
}

#if !TEST_R5

void
STXTTranslatorTest::LoadAddOnTest()
{
	// . Make sure the add_on loads
	NextSubTest();
	const char *path = "/boot/home/config/add-ons/Translators/STXTTranslator";
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
	
	// Run a number of tests on the BTranslator object
	TestBTranslator(this, ptran);
		// NOTE: this function Release()s ptran
	
	// . Unload Add-on
	NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK); 
}

#endif // #if !TEST_R5
