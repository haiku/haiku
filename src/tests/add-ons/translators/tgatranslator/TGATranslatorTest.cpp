// TGATranslatorTest.cpp
#include "TGATranslatorTest.h"
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
#include "TranslatorTestAddOn.h"

// Suite
CppUnit::Test *
TGATranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<TGATranslatorTest> TC;
			
	suite->addTest(
		new TC("TGATranslator IdentifyTest",
			&TGATranslatorTest::IdentifyTest));

	suite->addTest(
		new TC("TGATranslator TranslateTest",
			&TGATranslatorTest::TranslateTest));	

#if !TEST_R5
	suite->addTest(
		new TC("TGATranslator LoadAddOnTest",
			&TGATranslatorTest::LoadAddOnTest));
#endif
		
	return suite;
}		

// setUp
void
TGATranslatorTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
TGATranslatorTest::tearDown()
{
	BTestCase::tearDown();
}

void
CheckBits_Tga(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP,
		0.6f, 0.8f, "Be Bitmap Format (TGATranslator)",
		"image/x-be-bitmap");
}

void
CheckTga(translator_info *pti, const char *imageType)
{
	CheckTranslatorInfo(pti, B_TGA_FORMAT, B_TRANSLATOR_BITMAP,
		1.0f, 0.6f, imageType, "image/x-targa");
}

// coveniently group path of image with
// the expected Identify() string for that image
struct IdentifyInfo {
	const char *imagePath;
	const char *identifyString;
};

void
IdentifyTests(TGATranslatorTest *ptest, BTranslatorRoster *proster,
	const IdentifyInfo *pinfo, int32 len, bool bbits)
{
	translator_info ti;
	printf(" [%d] ", (int) bbits);
	
	for (int32 i = 0; i < len; i++) {
		ptest->NextSubTest();
		BFile file;
		printf(" [%s] ", pinfo[i].imagePath);
		CPPUNIT_ASSERT(file.SetTo(pinfo[i].imagePath, B_READ_ONLY) == B_OK);

		// Identify (output: B_TRANSLATOR_ANY_TYPE)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti) == B_OK);
		if (bbits)
			CheckBits_Tga(&ti);
		else
			CheckTga(&ti, pinfo[i].identifyString);
	
		// Identify (output: B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_BITMAP) == B_OK);
		if (bbits)
			CheckBits_Tga(&ti);
		else
			CheckTga(&ti, pinfo[i].identifyString);
	
		// Identify (output: B_TGA_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TGA_FORMAT) == B_OK);
		if (bbits)
			CheckBits_Tga(&ti);
		else
			CheckTga(&ti, pinfo[i].identifyString);
	}
}

void
TGATranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/TGATranslator") == B_OK);
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
	const IdentifyInfo aBitsPaths[] = {
		{ "/boot/home/resources/tga/screen1_16.bits", "" },
		{ "/boot/home/resources/tga/ugly_24_none_true.bits", "" }
	};
	const IdentifyInfo aTgaPaths[] = {
		{ "/boot/home/resources/tga/blocks_16_rle_true.tga",
			"Targa image (16 bits RLE truecolor)" },
		{ "/boot/home/resources/tga/blocks_24_none_true.tga",
			"Targa image (24 bits truecolor)" },
		{ "/boot/home/resources/tga/blocks_24_rle_true.tga",
			"Targa image (24 bits RLE truecolor)" },
		{ "/boot/home/resources/tga/blocks_8_none_gray.tga",
			"Targa image (8 bits gray)" },
		{ "/boot/home/resources/tga/blocks_8_rle_cmap.tga",
			"Targa image (8 bits RLE colormap)" },
		{ "/boot/home/resources/tga/cloudcg_16_none_true.tga",
			"Targa image (16 bits truecolor)" },
		{ "/boot/home/resources/tga/cloudcg_16_rle_true.tga",
			"Targa image (16 bits RLE truecolor)" },
		{ "/boot/home/resources/tga/cloudcg_24_none_true.tga",
			"Targa image (24 bits truecolor)" },
		{ "/boot/home/resources/tga/cloudcg_24_rle_true.tga",
			"Targa image (24 bits RLE truecolor)" },
		{ "/boot/home/resources/tga/cloudcg_8_none_cmap.tga",
			"Targa image (8 bits colormap)" },
		{ "/boot/home/resources/tga/cloudcg_8_none_gray.tga",
			"Targa image (8 bits gray)" },
		{ "/boot/home/resources/tga/cloudcg_8_rle_cmap.tga",
			"Targa image (8 bits RLE colormap)" },
		{ "/boot/home/resources/tga/graycloudcg_8_rle_cmap.tga",
			"Targa image (8 bits RLE colormap)" },
		{ "/boot/home/resources/tga/grayblocks_8_rle_cmap.tga",
			"Targa image (8 bits RLE colormap)" },
		{ "/boot/home/resources/tga/screen1_16_none_true.tga",
			"Targa image (16 bits truecolor)" },
		{ "/boot/home/resources/tga/screen1_16_rle_true.tga",
			"Targa image (16 bits RLE truecolor)" },
		{ "/boot/home/resources/tga/screen1_24_none_true.tga",
			"Targa image (24 bits truecolor)" },
		{ "/boot/home/resources/tga/screen1_24_rle_true.tga",
			"Targa image (24 bits RLE truecolor)" },
		{ "/boot/home/resources/tga/screen1_8_none_cmap.tga",
			"Targa image (8 bits colormap)" },
		{ "/boot/home/resources/tga/screen1_8_none_gray.tga",
			"Targa image (8 bits gray)" },
		{ "/boot/home/resources/tga/screen1_8_rle_cmap.tga",
			"Targa image (8 bits RLE colormap)" },
		{ "/boot/home/resources/tga/grayscreen1_8_rle_cmap.tga",
			"Targa image (8 bits RLE colormap)" },
		{ "/boot/home/resources/tga/ugly_16_none_true.tga",
			"Targa image (16 bits truecolor)" },
		{ "/boot/home/resources/tga/ugly_24_none_true.tga",
			"Targa image (24 bits truecolor)" },
		{ "/boot/home/resources/tga/ugly_32_none_true.tga",
			"Targa image (32 bits truecolor)" },
		{ "/boot/home/resources/tga/ugly_8_none_cmap.tga",
			"Targa image (8 bits colormap)" }
	};

	IdentifyTests(this, proster, aTgaPaths,
		sizeof(aTgaPaths) / sizeof(IdentifyInfo), false);
	IdentifyTests(this, proster, aBitsPaths,
		sizeof(aBitsPaths) / sizeof(IdentifyInfo), true);
	
	delete proster;
	proster = NULL;
}

// coveniently group path of tga image with
// path of bits image that it should translate to
struct TranslatePaths {
	const char *tgaPath;
	const char *bitsPath;
};

void
TranslateTests(TGATranslatorTest *ptest, BTranslatorRoster *proster,
	const TranslatePaths *paths, int32 len)
{
	// Perform translations on every file in the array
	for (int32 i = 0; i < len; i++) {
		// Setup input files	
		ptest->NextSubTest();
		BFile tga_file, bits_file;
		CPPUNIT_ASSERT(tga_file.SetTo(paths[i].tgaPath, B_READ_ONLY) == B_OK);
		CPPUNIT_ASSERT(bits_file.SetTo(paths[i].bitsPath, B_READ_ONLY) == B_OK);
		printf(" [%s] ", paths[i].tgaPath);
		
		BMallocIO mallio, dmallio;
		
		// Convert to B_TRANSLATOR_ANY_TYPE (should be B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tga_file, NULL, NULL, &mallio,
			B_TRANSLATOR_ANY_TYPE) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bits_file) == true);
		
		// Convert to B_TRANSLATOR_BITMAP
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tga_file, NULL, NULL, &mallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bits_file) == true);
		
		// Convert bits mallio to B_TRANSLATOR_BITMAP dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bits_file) == true);
		
		// Convert to B_TGA_FORMAT
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tga_file, NULL, NULL, &mallio,
			B_TGA_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, tga_file) == true);
		
		// Convert TGA mallio to B_TRANSLATOR_BITMAP dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bits_file) == true);
		
		// Convert TGA mallio to B_TGA_FORMAT dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TGA_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, tga_file) == true);
	}
}

void
TGATranslatorTest::TranslateTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	off_t filesize = -1;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/TGATranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
	BFile output("/tmp/tga_test.out", B_WRITE_ONLY | 
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
		B_TGA_FORMAT);
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
	
	// Translate TGA images to bits
	const TranslatePaths aPaths[] = {
		{ "/boot/home/resources/tga/blocks_16_rle_true.tga",
			"/boot/home/resources/tga/blocks_color.bits" },
		{ "/boot/home/resources/tga/blocks_24_none_true.tga",
			"/boot/home/resources/tga/blocks_color.bits" },
		{ "/boot/home/resources/tga/blocks_24_rle_true.tga",
			"/boot/home/resources/tga/blocks_color.bits" },
		{ "/boot/home/resources/tga/blocks_8_none_gray.tga",
			"/boot/home/resources/tga/blocks_gray.bits" },
		{ "/boot/home/resources/tga/blocks_8_rle_cmap.tga",
			"/boot/home/resources/tga/blocks_color.bits" },
		{ "/boot/home/resources/tga/cloudcg_16_none_true.tga",
			"/boot/home/resources/tga/cloudcg_16.bits" },
		{ "/boot/home/resources/tga/cloudcg_16_rle_true.tga",
			"/boot/home/resources/tga/cloudcg_16.bits" },
		{ "/boot/home/resources/tga/cloudcg_24_none_true.tga",
			"/boot/home/resources/tga/cloudcg_24.bits" },
		{ "/boot/home/resources/tga/cloudcg_24_rle_true.tga",
			"/boot/home/resources/tga/cloudcg_24.bits" },
		{ "/boot/home/resources/tga/cloudcg_8_none_cmap.tga",
			"/boot/home/resources/tga/cloudcg_8_cmap.bits" },
		{ "/boot/home/resources/tga/cloudcg_8_none_gray.tga",
			"/boot/home/resources/tga/cloudcg_8_gray.bits" },
		{ "/boot/home/resources/tga/cloudcg_8_rle_cmap.tga",
			"/boot/home/resources/tga/cloudcg_8_cmap.bits" },
		{ "/boot/home/resources/tga/graycloudcg_8_rle_cmap.tga",
			"/boot/home/resources/tga/cloudcg_8_gray.bits" },
		{ "/boot/home/resources/tga/grayblocks_8_rle_cmap.tga",
			"/boot/home/resources/tga/blocks_gray.bits" },
		{ "/boot/home/resources/tga/screen1_16_none_true.tga",
			"/boot/home/resources/tga/screen1_16.bits" },
		{ "/boot/home/resources/tga/screen1_16_rle_true.tga",
			"/boot/home/resources/tga/screen1_16.bits" },
		{ "/boot/home/resources/tga/screen1_24_none_true.tga",
			"/boot/home/resources/tga/screen1_24.bits" },
		{ "/boot/home/resources/tga/screen1_24_rle_true.tga",
			"/boot/home/resources/tga/screen1_24.bits" },
		{ "/boot/home/resources/tga/screen1_8_none_cmap.tga",
			"/boot/home/resources/tga/screen1_8_cmap.bits" },
		{ "/boot/home/resources/tga/screen1_8_none_gray.tga",
			"/boot/home/resources/tga/screen1_8_gray.bits" },
		{ "/boot/home/resources/tga/screen1_8_rle_cmap.tga",
			"/boot/home/resources/tga/screen1_8_cmap.bits" },
		{ "/boot/home/resources/tga/grayscreen1_8_rle_cmap.tga",
			"/boot/home/resources/tga/screen1_8_gray.bits" },
		{ "/boot/home/resources/tga/ugly_16_none_true.tga",
			"/boot/home/resources/tga/ugly_16_none_true.bits" },
		{ "/boot/home/resources/tga/ugly_24_none_true.tga",
			"/boot/home/resources/tga/ugly_24_none_true.bits" },
		{ "/boot/home/resources/tga/ugly_32_none_true.tga",
			"/boot/home/resources/tga/ugly_32_none_true.bits" },
		{ "/boot/home/resources/tga/ugly_8_none_cmap.tga",
			"/boot/home/resources/tga/ugly_8_none_cmap.bits" }
	};
	
	TranslateTests(this, proster, aPaths,
		sizeof(aPaths) / sizeof(TranslatePaths));
	
	delete proster;
	proster = NULL;
}

#if !TEST_R5

// The input formats that this translator is supposed to support
translation_format gTGAInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.6f, // quality
		0.8f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (TGATranslator)"
	},
	{
		B_TGA_FORMAT,
		B_TRANSLATOR_BITMAP,
		1.0f,
		0.6f,
		"image/x-targa",
		"Targa image"
	}
};

// The output formats that this translator is supposed to support
translation_format gTGAOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.6f, // quality
		0.8f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (TGATranslator)"
	},
	{
		B_TGA_FORMAT,
		B_TRANSLATOR_BITMAP,
		1.0f,
		0.7f,
		"image/x-targa",
		"Targa image"
	}
};

void
TGATranslatorTest::LoadAddOnTest()
{
	TranslatorLoadAddOnTest("/boot/home/config/add-ons/Translators/TGATranslator",
		this,
		gTGAInputFormats, sizeof(gTGAInputFormats) / sizeof(translation_format),
		gTGAOutputFormats, sizeof(gTGAOutputFormats) / sizeof(translation_format));
}

#endif // #if !TEST_R5
