// PNGTranslatorTest.cpp
//
// NOTE: Most of the PNG images used in this test are from PNGSuite:
// http://www.schaik.com/pngsuite/pngsuite.html
#include "PNGTranslatorTest.h"
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
#include <String.h>
#include <File.h>
#include <DataIO.h>
#include <Errors.h>
#include <OS.h>
#include "TranslatorTestAddOn.h"

// PNG Translator Settings
#define PNG_SETTING_INTERLACE "png /interlace"

#define PNG_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)

#define PNG_IN_QUALITY 0.8
#define PNG_IN_CAPABILITY 0.8
#define PNG_OUT_QUALITY 0.8
#define PNG_OUT_CAPABILITY 0.5

#define BBT_IN_QUALITY 0.8
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.5
#define BBT_OUT_CAPABILITY 0.4

// Test Images Directory
#define IMAGES_DIR "/boot/home/resources/png/"

// Suite
CppUnit::Test *
PNGTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<PNGTranslatorTest> TC;
			
	suite->addTest(
		new TC("PNGTranslator IdentifyTest",
			&PNGTranslatorTest::IdentifyTest));

	suite->addTest(
		new TC("PNGTranslator TranslateTest",
			&PNGTranslatorTest::TranslateTest));	

#if !TEST_R5
	suite->addTest(
		new TC("PNGTranslator LoadAddOnTest",
			&PNGTranslatorTest::LoadAddOnTest));
#endif
		
	return suite;
}		

// setUp
void
PNGTranslatorTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
PNGTranslatorTest::tearDown()
{
	BTestCase::tearDown();
}

void
CheckBits_PNG(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY, BBT_IN_CAPABILITY, "Be Bitmap Format (PNGTranslator)",
		"image/x-be-bitmap");
}

void
CheckPNG(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_PNG_FORMAT, B_TRANSLATOR_BITMAP,
		PNG_IN_QUALITY, PNG_IN_CAPABILITY, "PNG image", "image/png");
}

void
IdentifyTests(PNGTranslatorTest *ptest, BTranslatorRoster *proster,
	const char **paths, int32 len, bool bbits)
{
	translator_info ti;
	printf(" [%d] ", (int) bbits);
	
	BString fullpath;
	
	for (int32 i = 0; i < len; i++) {
		ptest->NextSubTest();
		BFile file;
		fullpath = IMAGES_DIR;
		fullpath += paths[i];
		printf(" [%s] ", fullpath.String());
		CPPUNIT_ASSERT(file.SetTo(fullpath.String(), B_READ_ONLY) == B_OK);

		// Identify (output: B_TRANSLATOR_ANY_TYPE)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti) == B_OK);
		if (bbits)
			CheckBits_PNG(&ti);
		else
			CheckPNG(&ti);
	
		// Identify (output: B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_BITMAP) == B_OK);
		if (bbits)
			CheckBits_PNG(&ti);
		else
			CheckPNG(&ti);
	
		// Identify (output: B_PNG_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_PNG_FORMAT) == B_OK);
		if (bbits)
			CheckBits_PNG(&ti);
		else
			CheckPNG(&ti);
	}
}

void
PNGTranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/PNGTranslator") == B_OK);
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
	
	// Identify (bad PNG signature)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	BFile badsig1(IMAGES_DIR "xlfn0g04.png", B_READ_ONLY);
	CPPUNIT_ASSERT(badsig1.InitCheck() == B_OK);
	result = proster->Identify(&badsig1, NULL, &ti);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	// Identify (bad PNG signature)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	BFile badsig2(IMAGES_DIR "/xcrn0g04.png", B_READ_ONLY);
	CPPUNIT_ASSERT(badsig2.InitCheck() == B_OK);
	result = proster->Identify(&badsig2, NULL, &ti);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
	
	// Identify (successfully identify the following files)
	const char * aBitsPaths[] = {
		"beer.bits",
		"blocks.bits"
	};
	const char * aPNGPaths[] = {
		"basi0g01.png",
		"basi0g02.png",
		"basn0g01.png",
		"basn0g04.png",
		"basi0g16.png",
		"basi4a08.png",
		"basn0g08.png",
		"basi4a16.png",
		"tp1n3p08.png",
		"tp0n2c08.png",
		"tbgn2c16.png",
		"s39i3p04.png",
		"basi6a08.png",
		"basi6a16.png",
		"basn6a08.png",
		"basi3p01.png",
		"basn3p02.png"
	};

	IdentifyTests(this, proster, aPNGPaths,
		sizeof(aPNGPaths) / sizeof(const char *), false);
	IdentifyTests(this, proster, aBitsPaths,
		sizeof(aBitsPaths) / sizeof(const char *), true);
	
	delete proster;
	proster = NULL;
}

// coveniently group path of PNG image with
// path of bits image that it should translate to
struct TranslatePaths {
	const char *pngPath;
	const char *bitsPath;
};

void
TranslateTests(PNGTranslatorTest *ptest, BTranslatorRoster *proster,
	const TranslatePaths *paths, int32 len)
{
	BString png_fpath, bits_fpath;
	
	// Perform translations on every file in the array
	for (int32 i = 0; i < len; i++) {
		// Setup input files	
		ptest->NextSubTest();
		png_fpath = bits_fpath = IMAGES_DIR;
		png_fpath += paths[i].pngPath;
		bits_fpath += paths[i].bitsPath;
		BFile png_file, bits_file;
		CPPUNIT_ASSERT(png_file.SetTo(png_fpath.String(), B_READ_ONLY) == B_OK);
		CPPUNIT_ASSERT(bits_file.SetTo(bits_fpath.String(), B_READ_ONLY) == B_OK);
		printf(" [%s] ", png_fpath.String());
		
		BMallocIO mallio, dmallio;
		
		// Convert to B_TRANSLATOR_ANY_TYPE (should be B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&png_file, NULL, NULL, &mallio,
			B_TRANSLATOR_ANY_TYPE) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bits_file) == true);
		
		// Convert to B_TRANSLATOR_BITMAP
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&png_file, NULL, NULL, &mallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bits_file) == true);
		
		// Convert bits mallio to B_TRANSLATOR_BITMAP dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bits_file) == true);
		
		// Convert to B_PNG_FORMAT
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&png_file, NULL, NULL, &mallio,
			B_PNG_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, png_file) == true);
		
		// Convert PNG mallio to B_TRANSLATOR_BITMAP dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bits_file) == true);
		
		// Convert PNG mallio to B_PNG_FORMAT dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_PNG_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, png_file) == true);
	}
}

void
PNGTranslatorTest::TranslateTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	off_t filesize = -1;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/PNGTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
	BFile output("/tmp/png_test.out", B_WRITE_ONLY | 
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
		B_PNG_FORMAT);
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
	
	// Translate (bad PNG signature)
	NextSubTest();
	BFile badsig1(IMAGES_DIR "xlfn0g04.png", B_READ_ONLY);
	CPPUNIT_ASSERT(badsig1.InitCheck() == B_OK);
	result = proster->Translate(&badsig1, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
	
	// Translate (bad PNG signature)
	NextSubTest();
	BFile badsig2(IMAGES_DIR "xcrn0g04.png", B_READ_ONLY);
	CPPUNIT_ASSERT(badsig2.InitCheck() == B_OK);
	result = proster->Translate(&badsig2, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);

	// Translate (bad width)
	NextSubTest();
	BFile badw(IMAGES_DIR "x00n0g01.png", B_READ_ONLY);
	CPPUNIT_ASSERT(badw.InitCheck() == B_OK);
	result = proster->Translate(&badw, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_ERROR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
	
	// Translate PNG images to bits
	const TranslatePaths aPaths[] = {
		{ "basi0g01.png", "basi0g01.bits" },
		{ "basi0g02.png", "basi0g02.bits" },
		{ "basn0g01.png", "basn0g01.bits" },
		{ "basn0g04.png", "basn0g04.bits" },
		{ "basi0g16.png", "basi0g16.bits" },
		{ "basi4a08.png", "basi4a08.bits" },
		{ "basn0g08.png", "basn0g08.bits" },
		{ "basi4a16.png", "basi4a16.bits" },
		{ "tp1n3p08.png", "tp1n3p08.bits" },
		{ "tp0n2c08.png", "tp0n2c08.bits" },
		{ "tbgn2c16.png", "tbgn2c16.bits" },
		{ "s39i3p04.png", "s39i3p04.bits" },
		{ "basi6a08.png", "basi6a08.bits" },
		{ "basi6a16.png", "basi6a16.bits" },
		{ "basn6a08.png", "basn6a08.bits" },
		{ "basi3p01.png", "basi3p01.bits" },
		{ "basn3p02.png", "basn3p02.bits" }
	};
	
	TranslateTests(this, proster, aPaths,
		sizeof(aPaths) / sizeof(TranslatePaths));
	
	delete proster;
	proster = NULL;
}

#if !TEST_R5

// The input formats that this translator supports.
translation_format gPNGInputFormats[] = {
	{
		B_PNG_FORMAT,
		B_TRANSLATOR_BITMAP,
		PNG_IN_QUALITY,
		PNG_IN_CAPABILITY,
		"image/png",
		"PNG image"
	},
	{
		B_PNG_FORMAT,
		B_TRANSLATOR_BITMAP,
		PNG_IN_QUALITY,
		PNG_IN_CAPABILITY,
		"image/x-png",
		"PNG image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (PNGTranslator)"
	}
};

// The output formats that this translator supports.
translation_format gPNGOutputFormats[] = {
	{
		B_PNG_FORMAT,
		B_TRANSLATOR_BITMAP,
		PNG_OUT_QUALITY,
		PNG_OUT_CAPABILITY,
		"image/png",
		"PNG image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_OUT_QUALITY,
		BBT_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (PNGTranslator)"
	}
};

void
PNGTranslatorTest::LoadAddOnTest()
{
	TranslatorLoadAddOnTest("/boot/home/config/add-ons/Translators/PNGTranslator",
		this,
		gPNGInputFormats, sizeof(gPNGInputFormats) / sizeof(translation_format),
		gPNGOutputFormats, sizeof(gPNGOutputFormats) / sizeof(translation_format),
		PNG_TRANSLATOR_VERSION);
}

#endif // #if !TEST_R5
