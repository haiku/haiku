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
#include "TranslatorTestAddOn.h"

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
CheckBits_Tiff(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP,
		0.7f, 0.6f, "Be Bitmap Format (TIFFTranslator)",
		"image/x-be-bitmap");
}

void
CheckTiff(translator_info *pti, const char *imageType)
{
	CheckTranslatorInfo(pti, B_TIFF_FORMAT, B_TRANSLATOR_BITMAP,
		0.7f, 0.6f, imageType, "image/tiff");
}

// coveniently group path of image with
// the expected Identify() string for that image
struct IdentifyInfo {
	const char *imagePath;
	const char *identifyString;
};

void
IdentifyTests(TIFFTranslatorTest *ptest, BTranslatorRoster *proster,
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
			CheckBits_Tiff(&ti);
		else
			CheckTiff(&ti, pinfo[i].identifyString);
	
		// Identify (output: B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_BITMAP) == B_OK);
		if (bbits)
			CheckBits_Tiff(&ti);
		else
			CheckTiff(&ti, pinfo[i].identifyString);
	
		// Identify (output: B_TIFF_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TIFF_FORMAT) == B_OK);
		if (bbits)
			CheckBits_Tiff(&ti);
		else
			CheckTiff(&ti, pinfo[i].identifyString);
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
	const IdentifyInfo aBitsPaths[] = {
		{ "/boot/home/resources/tiff/beer.bits", "" },
		{ "/boot/home/resources/tiff/blocks.bits", "" }
	};
	const IdentifyInfo aTiffPaths[] = {
		{ "/boot/home/resources/tiff/beer_rgb_nocomp.tif",
			"TIFF image (Little, RGB, None)" },
		{ "/boot/home/resources/tiff/beer_rgb_nocomp_big.tif",
			"TIFF image (Big, RGB, None)" },
		{ "/boot/home/resources/tiff/blocks_rgb_nocomp.tif",
			"TIFF image (Little, RGB, None)" },
		{ "/boot/home/resources/tiff/hills_bw_huffman.tif",
			"TIFF image (Little, Mono, Huffman)" },
		{ "/boot/home/resources/tiff/hills_cmyk_nocomp.tif",
			"TIFF image (Little, CMYK, None)" },
		{ "/boot/home/resources/tiff/hills_cmyk_nocomp_big.tif",
			"TIFF image (Big, CMYK, None)" },
		{ "/boot/home/resources/tiff/hills_rgb_nocomp.tif",
			"TIFF image (Little, RGB, None)" },
		{ "/boot/home/resources/tiff/hills_rgb_packbits.tif",
			"TIFF image (Little, RGB, PackBits)" },
		{ "/boot/home/resources/tiff/homes_bw_fax.tif",
			"TIFF image (Little, Mono, Group 3)" },
		{ "/boot/home/resources/tiff/homes_bw_huffman.tif",
			"TIFF image (Little, Mono, Huffman)" },
		{ "/boot/home/resources/tiff/homes_bw_nocomp.tif",
			"TIFF image (Little, Mono, None)" },
		{ "/boot/home/resources/tiff/homes_bw_nocomp_big.tif",
			"TIFF image (Big, Mono, None)" },
		{ "/boot/home/resources/tiff/homes_bw_packbits.tif",
			"TIFF image (Little, Mono, PackBits)" },
		{ "/boot/home/resources/tiff/homes_cmap4_nocomp.tif",
			"TIFF image (Little, Palette, None)" },
		{ "/boot/home/resources/tiff/homes_cmap4_nocomp_big.tif",
			"TIFF image (Big, Palette, None)" },
		{ "/boot/home/resources/tiff/homes_cmap4_packbits.tif",
			"TIFF image (Little, Palette, PackBits)" },
		{ "/boot/home/resources/tiff/homes_gray8_nocomp.tif",
			"TIFF image (Little, Gray, None)" },
		{ "/boot/home/resources/tiff/homes_gray8_nocomp_big.tif",
			"TIFF image (Big, Gray, None)" },
		{ "/boot/home/resources/tiff/homes_gray8_packbits.tif",
			"TIFF image (Little, Gray, PackBits)" },
		{ "/boot/home/resources/tiff/logo_cmap4_nocomp.tif",
			"TIFF image (Little, Palette, None)" },
		{ "/boot/home/resources/tiff/logo_cmap4_nocomp_big.tif",
			"TIFF image (Big, Palette, None)" },
		{ "/boot/home/resources/tiff/logo_cmap4_packbits.tif",
			"TIFF image (Little, Palette, PackBits)" },
		{ "/boot/home/resources/tiff/logo_cmap8_nocomp.tif",
			"TIFF image (Little, Palette, None)" },
		{ "/boot/home/resources/tiff/logo_cmap8_nocomp_big.tif",
			"TIFF image (Big, Palette, None)" },
		{ "/boot/home/resources/tiff/logo_cmap8_packbits.tif",
			"TIFF image (Little, Palette, PackBits)" },
		{ "/boot/home/resources/tiff/logo_cmyk_nocomp.tif",
			"TIFF image (Little, CMYK, None)" },
		{ "/boot/home/resources/tiff/vsmall_cmap4_nocomp.tif",
			"TIFF image (Little, Palette, None)" },
		{ "/boot/home/resources/tiff/vsmall_rgb_nocomp.tif",
			"TIFF image (Little, RGB, None)" },
		{ "/boot/home/resources/tiff/backup_help.tif",
			"TIFF image (Big, Mono, Group 3)" }
	};

	IdentifyTests(this, proster, aTiffPaths,
		sizeof(aTiffPaths) / sizeof(IdentifyInfo), false);
	IdentifyTests(this, proster, aBitsPaths,
		sizeof(aBitsPaths) / sizeof(IdentifyInfo), true);
	
	delete proster;
	proster = NULL;
}

// coveniently group path of tiff image with
// path of bits image that it should translate to
struct TranslatePaths {
	const char *tiffPath;
	const char *bitsPath;
};

void
TranslateTests(TIFFTranslatorTest *ptest, BTranslatorRoster *proster,
	const TranslatePaths *paths, int32 len)
{
	// Perform translations on every file in the array
	for (int32 i = 0; i < len; i++) {
		// Setup input files	
		ptest->NextSubTest();
		BFile tiff_file, bits_file;
		CPPUNIT_ASSERT(tiff_file.SetTo(paths[i].tiffPath, B_READ_ONLY) == B_OK);
		CPPUNIT_ASSERT(bits_file.SetTo(paths[i].bitsPath, B_READ_ONLY) == B_OK);
		printf(" [%s] ", paths[i].tiffPath);
		
		BMallocIO mallio, dmallio;
		
		// Convert to B_TRANSLATOR_ANY_TYPE (should be B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tiff_file, NULL, NULL, &mallio,
			B_TRANSLATOR_ANY_TYPE) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bits_file) == true);
		
		// Convert to B_TRANSLATOR_BITMAP
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tiff_file, NULL, NULL, &mallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bits_file) == true);
		
		// Convert bits mallio to B_TRANSLATOR_BITMAP dmallio
		/* Not Supported Yet
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bits_file) == true);
		*/
		
		// Convert to B_TIFF_FORMAT
		/* Not Supported Yet
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tiff_file, NULL, NULL, &mallio,
			B_TIFF_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, tiff_file) == true);
		*/
		
		// Convert TIFF mallio to B_TRANSLATOR_BITMAP dmallio
		/* Not Ready Yet
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bits_file) == true);
		*/
		
		// Convert TIFF mallio to B_TIFF_FORMAT dmallio
		/* Not Ready Yet
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TIFF_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, tiff_file) == true);
		*/
	}
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
	
	// Translate TIFF images to bits
	const TranslatePaths aPaths[] = {
		{ "/boot/home/resources/tiff/beer_rgb_nocomp.tif",
			"/boot/home/resources/tiff/beer.bits" },
		{ "/boot/home/resources/tiff/beer_rgb_nocomp_big.tif",
			"/boot/home/resources/tiff/beer.bits" },
		{ "/boot/home/resources/tiff/blocks_rgb_nocomp.tif",
			"/boot/home/resources/tiff/blocks.bits" },
		{ "/boot/home/resources/tiff/hills_bw_huffman.tif",
			"/boot/home/resources/tiff/hills_bw.bits" },
		{ "/boot/home/resources/tiff/hills_cmyk_nocomp.tif",
			"/boot/home/resources/tiff/hills_cmyk.bits" },
		{ "/boot/home/resources/tiff/hills_cmyk_nocomp_big.tif",
			"/boot/home/resources/tiff/hills_cmyk.bits" },
		{ "/boot/home/resources/tiff/hills_rgb_nocomp.tif",
			"/boot/home/resources/tiff/hills_rgb.bits" },
		{ "/boot/home/resources/tiff/hills_rgb_packbits.tif",
			"/boot/home/resources/tiff/hills_rgb.bits" },
		{ "/boot/home/resources/tiff/homes_bw_fax.tif",
			"/boot/home/resources/tiff/homes_bw.bits" },
		{ "/boot/home/resources/tiff/homes_bw_huffman.tif",
			"/boot/home/resources/tiff/homes_bw.bits" },
		{ "/boot/home/resources/tiff/homes_bw_nocomp.tif",
			"/boot/home/resources/tiff/homes_bw.bits" },
		{ "/boot/home/resources/tiff/homes_bw_nocomp_big.tif",
			"/boot/home/resources/tiff/homes_bw.bits" },
		{ "/boot/home/resources/tiff/homes_bw_packbits.tif",
			"/boot/home/resources/tiff/homes_bw.bits" },
		{ "/boot/home/resources/tiff/homes_cmap4_nocomp.tif",
			"/boot/home/resources/tiff/homes_cmap4.bits" },
		{ "/boot/home/resources/tiff/homes_cmap4_nocomp_big.tif",
			"/boot/home/resources/tiff/homes_cmap4.bits" },
		{ "/boot/home/resources/tiff/homes_cmap4_packbits.tif",
			"/boot/home/resources/tiff/homes_cmap4.bits" },
		{ "/boot/home/resources/tiff/homes_gray8_nocomp.tif",
			"/boot/home/resources/tiff/homes_gray8.bits" },
		{ "/boot/home/resources/tiff/homes_gray8_nocomp_big.tif",
			"/boot/home/resources/tiff/homes_gray8.bits" },
		{ "/boot/home/resources/tiff/homes_gray8_packbits.tif",
			"/boot/home/resources/tiff/homes_gray8.bits" },
		{ "/boot/home/resources/tiff/logo_cmap4_nocomp.tif",
			"/boot/home/resources/tiff/logo_cmap4.bits" },
		{ "/boot/home/resources/tiff/logo_cmap4_nocomp_big.tif",
			"/boot/home/resources/tiff/logo_cmap4.bits" },
		{ "/boot/home/resources/tiff/logo_cmap4_packbits.tif",
			"/boot/home/resources/tiff/logo_cmap4.bits" },
		{ "/boot/home/resources/tiff/logo_cmap8_nocomp.tif",
			"/boot/home/resources/tiff/logo_rgb.bits" },
		{ "/boot/home/resources/tiff/logo_cmap8_nocomp_big.tif",
			"/boot/home/resources/tiff/logo_rgb.bits" },
		{ "/boot/home/resources/tiff/logo_cmap8_packbits.tif",
			"/boot/home/resources/tiff/logo_rgb.bits" },
		{ "/boot/home/resources/tiff/logo_cmyk_nocomp.tif",
			"/boot/home/resources/tiff/logo_cmyk.bits" },
		{ "/boot/home/resources/tiff/vsmall_cmap4_nocomp.tif",
			"/boot/home/resources/tiff/vsmall.bits" },
		{ "/boot/home/resources/tiff/vsmall_rgb_nocomp.tif",
			"/boot/home/resources/tiff/vsmall.bits" },
		{ "/boot/home/resources/tiff/backup_help.tif",
			"/boot/home/resources/tiff/backup_help.bits" }
	};
	
	TranslateTests(this, proster, aPaths,
		sizeof(aPaths) / sizeof(TranslatePaths));
	
	delete proster;
	proster = NULL;
}

#if !TEST_R5

// The input formats that this translator is supposed to support
translation_format gTIFFInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.7f, // quality
		0.6f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		0.7f,
		0.6f,
		"image/tiff",
		"TIFF image"
	}
};

// The output formats that this translator is supposed to support
translation_format gTIFFOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.7f, // quality
		0.6f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		0.7f,
		0.6f,
		"image/tiff",
		"TIFF image"
	}
};

void
TIFFTranslatorTest::LoadAddOnTest()
{
	TranslatorLoadAddOnTest("/boot/home/config/add-ons/Translators/TIFFTranslator",
		this,
		gTIFFInputFormats, sizeof(gTIFFInputFormats) / sizeof(translation_format),
		gTIFFOutputFormats, sizeof(gTIFFOutputFormats) / sizeof(translation_format),
		B_TRANSLATION_MAKE_VERSION(1,0,0));
}

#endif // #if !TEST_R5
