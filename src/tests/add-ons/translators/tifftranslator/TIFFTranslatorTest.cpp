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
CheckBits(translator_info *pti)
{
	CPPUNIT_ASSERT(pti->type == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(pti->quality > 0.39 && pti->quality < 0.41);
	CPPUNIT_ASSERT(pti->capability > 0.59 && pti->capability < 0.61);
	CPPUNIT_ASSERT(strcmp(pti->name, "Be Bitmap Format (TIFFTranslator)") == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, "image/x-be-bitmap") == 0);
}

void
CheckTiff(translator_info *pti, const char *imageType)
{
	CPPUNIT_ASSERT(pti->type == B_TIFF_FORMAT);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(pti->quality > 0.09 && pti->quality < 0.11);
	CPPUNIT_ASSERT(pti->capability > 0.09 && pti->capability < 0.11);
	CPPUNIT_ASSERT(strcmp(pti->name, imageType) == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, "image/tiff") == 0);
}

void
IdentifyTests(TIFFTranslatorTest *ptest, BTranslatorRoster *proster,
	const char **paths, const char **imageTypes, int32 len, bool bbits)
{
	translator_info ti;
	printf(" [%d] ", (int) bbits);
	
	for (int32 i = 0; i < len; i++) {
		ptest->NextSubTest();
		BFile file;
		printf(" [%s] ", paths[i]);
		CPPUNIT_ASSERT(file.SetTo(paths[i], B_READ_ONLY) == B_OK);

		// Identify (output: B_TRANSLATOR_ANY_TYPE)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti) == B_OK);
		if (bbits)
			CheckBits(&ti);
		else
			CheckTiff(&ti, imageTypes[i]);
	
		// Identify (output: B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_BITMAP) == B_OK);
		if (bbits)
			CheckBits(&ti);
		else
			CheckTiff(&ti, imageTypes[i]);
	
		// Identify (output: B_TIFF_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TIFF_FORMAT) == B_OK);
		if (bbits)
			CheckBits(&ti);
		else
			CheckTiff(&ti, imageTypes[i]);
	}
}

void
TIFFTranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/TIFFTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
		
	// Identify (bad input, output types)
	NextSubTest();
	translator_info ti;
	memset(&ti, 0, sizeof(translator_info));
	result = proster->Identify(&wronginput, NULL, &ti, 0,
		NULL, B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	// Identify (wrong type of input data)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	result = proster->Identify(&wronginput, NULL, &ti);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	// Identify (successfully identify the following files)
	const char *aBitsFiles[] = {
		"/boot/home/resources/tiff/beer.bits",
		"/boot/home/resources/tiff/blocks.bits"
	};
	const char *aTiffFiles[] = {
		"/boot/home/resources/tiff/beer_rgb_nocomp.tif",
		"/boot/home/resources/tiff/beer_rgb_nocomp_big.tif",
		"/boot/home/resources/tiff/blocks_rgb_nocomp.tif",
		"/boot/home/resources/tiff/hills_bw_huffman.tif",
		"/boot/home/resources/tiff/hills_cmyk_nocomp.tif",
		"/boot/home/resources/tiff/hills_cmyk_nocomp_big.tif",
		"/boot/home/resources/tiff/hills_rgb_nocomp.tif",
		"/boot/home/resources/tiff/hills_rgb_packbits.tif",
		"/boot/home/resources/tiff/homes_bw_huffman.tif",
		"/boot/home/resources/tiff/homes_bw_nocomp.tif",
		"/boot/home/resources/tiff/homes_bw_nocomp_big.tif",
		"/boot/home/resources/tiff/homes_bw_packbits.tif",
		"/boot/home/resources/tiff/homes_cmap4_nocomp.tif",
		"/boot/home/resources/tiff/homes_cmap4_nocomp_big.tif",
		"/boot/home/resources/tiff/homes_cmap4_packbits.tif",
		"/boot/home/resources/tiff/homes_gray8_nocomp.tif",
		"/boot/home/resources/tiff/homes_gray8_nocomp_big.tif",
		"/boot/home/resources/tiff/homes_gray8_packbits.tif",
		"/boot/home/resources/tiff/logo_cmap4_nocomp.tif",
		"/boot/home/resources/tiff/logo_cmap4_nocomp_big.tif",
		"/boot/home/resources/tiff/logo_cmap4_packbits.tif",
		"/boot/home/resources/tiff/logo_cmap8_nocomp.tif",
		"/boot/home/resources/tiff/logo_cmap8_nocomp_big.tif",
		"/boot/home/resources/tiff/logo_cmap8_packbits.tif",
		"/boot/home/resources/tiff/logo_cmyk_nocomp.tif",
		"/boot/home/resources/tiff/vsmall_cmap4_nocomp.tif",
		"/boot/home/resources/tiff/vsmall_rgb_nocomp.tif"
	};
	const char *aTiffTypes[] = {
		"TIFF Image (Little, RGB, None)",
		"TIFF Image (Big, RGB, None)",
		"TIFF Image (Little, RGB, None)",
		"TIFF Image (Little, Mono, Huffman)",
		"TIFF Image (Little, CMYK, None)",
		"TIFF Image (Big, CMYK, None)",
		"TIFF Image (Little, RGB, None)",
		"TIFF Image (Little, RGB, PackBits)",
		"TIFF Image (Little, Mono, Huffman)",
		"TIFF Image (Little, Mono, None)",
		"TIFF Image (Big, Mono, None)",
		"TIFF Image (Little, Mono, PackBits)",
		"TIFF Image (Little, Palette, None)",
		"TIFF Image (Big, Palette, None)",
		"TIFF Image (Little, Palette, PackBits)",
		"TIFF Image (Little, Gray, None)",
		"TIFF Image (Big, Gray, None)",
		"TIFF Image (Little, Gray, PackBits)",
		"TIFF Image (Little, Palette, None)",
		"TIFF Image (Big, Palette, None)",
		"TIFF Image (Little, Palette, PackBits)",
		"TIFF Image (Little, Palette, None)",
		"TIFF Image (Big, Palette, None)",
		"TIFF Image (Little, Palette, PackBits)",
		"TIFF Image (Little, CMYK, None)",
		"TIFF Image (Little, Palette, None)",
		"TIFF Image (Little, RGB, None)",
	};
	CPPUNIT_ASSERT((sizeof(aTiffFiles) / sizeof(const char *)) ==
		(sizeof(aTiffTypes) / sizeof(const char *)));

	IdentifyTests(this, proster, aTiffFiles, aTiffTypes,
		sizeof(aTiffFiles) / sizeof(const char *), false);
	IdentifyTests(this, proster, aBitsFiles, NULL,
		sizeof(aBitsFiles) / sizeof(const char *), true);
	
	delete proster;
	proster = NULL;
}

void
TIFFTranslatorTest::TranslateTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	off_t filesize = -1;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/TIFFTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
	BFile output("/tmp/tiff_test.out", B_WRITE_ONLY | 
		B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(output.InitCheck() == B_OK);
	
	// Translate (bad input, output types)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
	
	// Translate (wrong type of input data)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TIFF_FORMAT);
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
	
	delete proster;
	proster = NULL;
}

// Apply a number of tests to a BTranslator * to a TIFFTranslator object
void
TestBTranslator(TIFFTranslatorTest *ptest, BTranslator *ptran)
{	
	// The translator should only have one reference
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
	// Make sure Acquire returns a BTranslator even though its
	// already been Acquired once
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);
	
	// Acquired twice, refcount should be 2
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);
	
	// Release should return ptran because it is still acquired
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
	
	// A name would be nice
	ptest->NextSubTest();
	const char *tranname = ptran->TranslatorName();
	CPPUNIT_ASSERT(tranname);
	printf(" {%s} ", tranname);
	
	// More info would be nice
	ptest->NextSubTest();
	const char *traninfo = ptran->TranslatorInfo();
	CPPUNIT_ASSERT(traninfo);
	printf(" {%s} ", traninfo);
	
	// What version are you?
	// (when ver == 100, that means that version is 1.00)
	ptest->NextSubTest();
	int32 ver = ptran->TranslatorVersion();
	CPPUNIT_ASSERT((ver / 100) > 0);
	printf(" {%d} ", (int) ver);
	
	// Input formats?
	ptest->NextSubTest();
	{
		int32 incount = 0;
		const translation_format *pins = ptran->InputFormats(&incount);
		CPPUNIT_ASSERT(incount == 2);
		CPPUNIT_ASSERT(pins);
		// must support B_TIFF_FORMAT and B_TRANSLATOR_BITMAP formats
		for (int32 i = 0; i < incount; i++) {
			CPPUNIT_ASSERT(pins[i].group == B_TRANSLATOR_BITMAP);
			CPPUNIT_ASSERT(pins[i].MIME);
			CPPUNIT_ASSERT(pins[i].name);

			if (pins[i].type == B_TRANSLATOR_BITMAP) {
				CPPUNIT_ASSERT(pins[i].quality > 0 && pins[i].quality <= 1);
				CPPUNIT_ASSERT(pins[i].capability > 0 && pins[i].capability <= 1);
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, BBT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name,
					"Be Bitmap Format (TIFFTranslator)") == 0);
			} else if (pins[i].type == B_TIFF_FORMAT) {
				CPPUNIT_ASSERT(pins[i].quality > 0 && pins[i].quality <= 0.11);
				CPPUNIT_ASSERT(pins[i].capability > 0 && pins[i].capability <= 0.11);
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, TIFF_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name, "TIFF Image") == 0);
			} else
				CPPUNIT_ASSERT(false);
		}
	}
	
	// Output formats?
	ptest->NextSubTest();
	{
		int32 outcount = 0;
		const translation_format *pouts = ptran->OutputFormats(&outcount);
		CPPUNIT_ASSERT(outcount == 2);
		CPPUNIT_ASSERT(pouts);
		// must support B_TIFF_FORMAT and B_TRANSLATOR_BITMAP formats
		for (int32 i = 0; i < outcount; i++) {
			CPPUNIT_ASSERT(pouts[i].group == B_TRANSLATOR_BITMAP);
			CPPUNIT_ASSERT(pouts[i].MIME);
			CPPUNIT_ASSERT(pouts[i].name);
	
			if (pouts[i].type == B_TRANSLATOR_BITMAP) {
				CPPUNIT_ASSERT(pouts[i].quality > 0 && pouts[i].quality <= 1);
				CPPUNIT_ASSERT(pouts[i].capability > 0 && pouts[i].capability <= 1);
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, BBT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name,
					"Be Bitmap Format (TIFFTranslator)") == 0);
			} else if (pouts[i].type == B_TIFF_FORMAT) {
				CPPUNIT_ASSERT(pouts[i].quality > 0.59 && pouts[i].quality < 0.61);
				CPPUNIT_ASSERT(pouts[i].capability > 0.19 && pouts[i].capability < 0.21);
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, TIFF_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name, "TIFF Image") == 0);
			} else
				CPPUNIT_ASSERT(false);
		}
	}
	
	// Release should return NULL because Release has been called
	// as many times as it has been acquired
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
	ptran = NULL;
}

#if !TEST_R5

void
TIFFTranslatorTest::LoadAddOnTest()
{
	// Make sure the add_on loads
	NextSubTest();
	const char *path = "/boot/home/config/add-ons/Translators/TIFFTranslator";
	image_id image = load_add_on(path);
	CPPUNIT_ASSERT(image >= 0);
	
	// Load in function to make the object
	NextSubTest();
	BTranslator *(*pMakeNthTranslator)(int32 n,image_id you,uint32 flags,...);
	status_t err = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void **)&pMakeNthTranslator);
	CPPUNIT_ASSERT(!err);

	// Make sure the function returns a pointer to a BTranslator
	NextSubTest();
	BTranslator *ptran = pMakeNthTranslator(0, image, 0);
	CPPUNIT_ASSERT(ptran);
	
	// Make sure the function only returns one BTranslator
	NextSubTest();
	CPPUNIT_ASSERT(!pMakeNthTranslator(1, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(2, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(3, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(16, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(1023, image, 0));
	
	// Run a number of tests on the BTranslator object
	TestBTranslator(this, ptran);
		// NOTE: this function Release()s ptran
	ptran = NULL;
	
	// Unload Add-on
	NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK); 
}

#endif // #if !TEST_R5
