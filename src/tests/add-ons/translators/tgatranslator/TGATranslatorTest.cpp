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
	CPPUNIT_ASSERT(pti->type == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(pti->quality > 0.59 && pti->quality < 0.61);
	CPPUNIT_ASSERT(pti->capability > 0.79 && pti->capability < 0.81);
	CPPUNIT_ASSERT(strcmp(pti->name, "Be Bitmap Format (TGATranslator)") == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, "image/x-be-bitmap") == 0);
}

void
CheckTga(translator_info *pti, const char *imageType)
{
	CPPUNIT_ASSERT(pti->type == B_TGA_FORMAT);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(pti->quality > 0.99 && pti->quality < 1.01);
	CPPUNIT_ASSERT(pti->capability > 0.59 && pti->capability < 0.61);
	CPPUNIT_ASSERT(strcmp(pti->name, imageType) == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, "image/x-targa") == 0);
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
		{ "/boot/home/resources/tga/ugly.bits", "" }
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
			"/boot/home/resources/tga/ugly.bits" },
		{ "/boot/home/resources/tga/ugly_24_none_true.tga",
			"/boot/home/resources/tga/ugly.bits" },
		{ "/boot/home/resources/tga/ugly_32_none_true.tga",
			"/boot/home/resources/tga/ugly.bits" },
		{ "/boot/home/resources/tga/ugly_8_none_cmap.tga",
			"/boot/home/resources/tga/ugly.bits" }
	};
	
	TranslateTests(this, proster, aPaths,
		sizeof(aPaths) / sizeof(TranslatePaths));
	
	delete proster;
	proster = NULL;
}

// Apply a number of tests to a BTranslator * to a TGATranslator object
void
TestBTranslator(TGATranslatorTest *ptest, BTranslator *ptran)
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
		// must support B_TGA_FORMAT and B_TRANSLATOR_BITMAP formats
		for (int32 i = 0; i < incount; i++) {
			CPPUNIT_ASSERT(pins[i].group == B_TRANSLATOR_BITMAP);
			CPPUNIT_ASSERT(pins[i].MIME);
			CPPUNIT_ASSERT(pins[i].name);

			if (pins[i].type == B_TRANSLATOR_BITMAP) {
				CPPUNIT_ASSERT(pins[i].quality > 0.59 && pins[i].quality < 0.61);
				CPPUNIT_ASSERT(pins[i].capability > 0.79 && pins[i].capability < 0.81);
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, BBT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name,
					"Be Bitmap Format (TGATranslator)") == 0);
			} else if (pins[i].type == B_TGA_FORMAT) {
				CPPUNIT_ASSERT(pins[i].quality > 0.99 && pins[i].quality < 1.01);
				CPPUNIT_ASSERT(pins[i].capability > 0.59 && pins[i].capability < 0.61);
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, TGA_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name, "Targa image") == 0);
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
		// must support B_TGA_FORMAT and B_TRANSLATOR_BITMAP formats
		for (int32 i = 0; i < outcount; i++) {
			CPPUNIT_ASSERT(pouts[i].group == B_TRANSLATOR_BITMAP);
			CPPUNIT_ASSERT(pouts[i].MIME);
			CPPUNIT_ASSERT(pouts[i].name);
	
			if (pouts[i].type == B_TRANSLATOR_BITMAP) {
				CPPUNIT_ASSERT(pouts[i].quality > 0.59 && pouts[i].quality < 0.61);
				CPPUNIT_ASSERT(pouts[i].capability > 0.79 && pouts[i].capability < 0.81);
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, BBT_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name,
					"Be Bitmap Format (TGATranslator)") == 0);
			} else if (pouts[i].type == B_TGA_FORMAT) {
				CPPUNIT_ASSERT(pouts[i].quality > 0.99 && pouts[i].quality < 1.01);
				CPPUNIT_ASSERT(pouts[i].capability > 0.59 && pouts[i].capability < 0.61);
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, TGA_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name, "Targa image") == 0);
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
TGATranslatorTest::LoadAddOnTest()
{
	// Make sure the add_on loads
	NextSubTest();
	const char *path = "/boot/home/config/add-ons/Translators/TGATranslator";
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
