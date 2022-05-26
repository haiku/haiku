/*****************************************************************************/
// TGATranslatorTest
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TGATranslatorTest.cpp
//
// Unit testing code to test the Haiku TGATranslator
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#include "TGATranslatorTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <image.h>
#include <Application.h>
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
			
	suite->addTest(
		new TC("TGATranslator ConfigMessageTest",
			&TGATranslatorTest::ConfigMessageTest));

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
		0.7f, 0.6f, "Be Bitmap Format (TGATranslator)",
		"image/x-be-bitmap");
}

void
CheckTga(translator_info *pti, const char *imageType)
{
	CheckTranslatorInfo(pti, B_TGA_FORMAT, B_TRANSLATOR_BITMAP,
		0.7f, 0.8f, imageType, "image/x-targa");
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
		{ "/boot/home/resources/tga/screen1_24.bits", "" },
		{ "/boot/home/resources/tga/b_gray1-2.bits", "" },
		{ "/boot/home/resources/tga/b_gray1.bits", "" },
		{ "/boot/home/resources/tga/b_rgb15.bits", "" },
		{ "/boot/home/resources/tga/b_rgb16.bits", "" },
		{ "/boot/home/resources/tga/b_rgb32.bits", "" },
		{ "/boot/home/resources/tga/b_cmap8.bits", "" }
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
	bool bcompress;
};

void
TranslateTests(TGATranslatorTest *ptest, BTranslatorRoster *proster,
	const TranslatePaths *paths, int32 len, bool btgainput)
{
	// Setup BMessages for specifying TGATranslator settings
	const char *krleoption = "tga /rle";
	BMessage compmsg, nocompmsg, *pmsg;
	CPPUNIT_ASSERT(compmsg.AddBool(krleoption, true) == B_OK);
	CPPUNIT_ASSERT(nocompmsg.AddBool(krleoption, false) == B_OK);

	// Perform translations on every file in the array
	for (int32 i = 0; i < len; i++) {
		// Setup input files	
		ptest->NextSubTest();
		BFile tgafile, bitsfile, *pinput;
		CPPUNIT_ASSERT(tgafile.SetTo(paths[i].tgaPath, B_READ_ONLY) == B_OK);
		CPPUNIT_ASSERT(bitsfile.SetTo(paths[i].bitsPath, B_READ_ONLY) == B_OK);
		if (btgainput) {
			printf(" [%s] ", paths[i].tgaPath);
			pinput = &tgafile;
		} else {
			printf(" [%s] ", paths[i].bitsPath);
			pinput = &bitsfile;
		}
		
		// RLE compression option
		if (paths[i].bcompress)
			pmsg = &compmsg;
		else
			pmsg = &nocompmsg;
			
		// create temporary files where the translated data will
		// be stored
		BFile tmpfile("/tmp/tgatmp", B_READ_WRITE | B_CREATE_FILE |
			B_ERASE_FILE);
		BFile dtmpfile("/tmp/tgadtmp", B_READ_WRITE | B_CREATE_FILE |
			B_ERASE_FILE);
		CPPUNIT_ASSERT(tmpfile.InitCheck() == B_OK);
		CPPUNIT_ASSERT(dtmpfile.InitCheck() == B_OK);
		// Convert to B_TRANSLATOR_ANY_TYPE (should be B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		CPPUNIT_ASSERT(tmpfile.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(tmpfile.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput, NULL, pmsg, &tmpfile,
			B_TRANSLATOR_ANY_TYPE) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(tmpfile, bitsfile) == true);
		
		// Convert to B_TRANSLATOR_BITMAP
		ptest->NextSubTest();
		CPPUNIT_ASSERT(tmpfile.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(tmpfile.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput, NULL, pmsg, &tmpfile,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(tmpfile, bitsfile) == true);
		
		// Convert bits tmpfile to B_TRANSLATOR_BITMAP dtmpfile
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dtmpfile.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dtmpfile.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&tmpfile, NULL, pmsg, &dtmpfile,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dtmpfile, bitsfile) == true);
		
		// Convert to B_TGA_FORMAT
		ptest->NextSubTest();
		CPPUNIT_ASSERT(tmpfile.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(tmpfile.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput, NULL, pmsg, &tmpfile,
			B_TGA_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(tmpfile, tgafile) == true);
		
		if (btgainput || strstr(paths[i].bitsPath, "24")) {
			// Convert TGA tmpfile to B_TRANSLATOR_BITMAP dtmpfile
			ptest->NextSubTest();
			CPPUNIT_ASSERT(dtmpfile.Seek(0, SEEK_SET) == 0);
			CPPUNIT_ASSERT(dtmpfile.SetSize(0) == B_OK);
			CPPUNIT_ASSERT(proster->Translate(&tmpfile, NULL, pmsg, &dtmpfile,
				B_TRANSLATOR_BITMAP) == B_OK);
			CPPUNIT_ASSERT(CompareStreams(dtmpfile, bitsfile) == true);
		
			// Convert TGA tmpfile to B_TGA_FORMAT dtmpfile
			ptest->NextSubTest();
			CPPUNIT_ASSERT(dtmpfile.Seek(0, SEEK_SET) == 0);
			CPPUNIT_ASSERT(dtmpfile.SetSize(0) == B_OK);
			CPPUNIT_ASSERT(proster->Translate(&tmpfile, NULL, pmsg, &dtmpfile,
				B_TGA_FORMAT) == B_OK);
			CPPUNIT_ASSERT(CompareStreams(dtmpfile, tgafile) == true);
		}
	}
}

void
TGATranslatorTest::TranslateTest()
{
	BApplication
		app("application/x-vnd.OpenBeOS-TGATranslatorTest");
		
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
	const TranslatePaths aTgaInputs[] = {
		{ "/boot/home/resources/tga/blocks_16_rle_true.tga",
			"/boot/home/resources/tga/blocks_color.bits", false },
		{ "/boot/home/resources/tga/blocks_24_none_true.tga",
			"/boot/home/resources/tga/blocks_color.bits", false },
		{ "/boot/home/resources/tga/blocks_24_rle_true.tga",
			"/boot/home/resources/tga/blocks_color.bits", false },
		{ "/boot/home/resources/tga/blocks_8_none_gray.tga",
			"/boot/home/resources/tga/blocks_gray.bits", false },
		{ "/boot/home/resources/tga/blocks_8_rle_cmap.tga",
			"/boot/home/resources/tga/blocks_color.bits", false },
		{ "/boot/home/resources/tga/cloudcg_16_none_true.tga",
			"/boot/home/resources/tga/cloudcg_16.bits", false },
		{ "/boot/home/resources/tga/cloudcg_16_rle_true.tga",
			"/boot/home/resources/tga/cloudcg_16.bits", false },
		{ "/boot/home/resources/tga/cloudcg_24_none_true.tga",
			"/boot/home/resources/tga/cloudcg_24.bits", false },
		{ "/boot/home/resources/tga/cloudcg_24_rle_true.tga",
			"/boot/home/resources/tga/cloudcg_24.bits", false },
		{ "/boot/home/resources/tga/cloudcg_8_none_cmap.tga",
			"/boot/home/resources/tga/cloudcg_8_cmap.bits", false },
		{ "/boot/home/resources/tga/cloudcg_8_none_gray.tga",
			"/boot/home/resources/tga/cloudcg_8_gray.bits", false },
		{ "/boot/home/resources/tga/cloudcg_8_rle_cmap.tga",
			"/boot/home/resources/tga/cloudcg_8_cmap.bits", false },
		{ "/boot/home/resources/tga/graycloudcg_8_rle_cmap.tga",
			"/boot/home/resources/tga/cloudcg_8_gray.bits", false },
		{ "/boot/home/resources/tga/grayblocks_8_rle_cmap.tga",
			"/boot/home/resources/tga/blocks_gray.bits", false },
		{ "/boot/home/resources/tga/screen1_16_none_true.tga",
			"/boot/home/resources/tga/screen1_16.bits", false },
		{ "/boot/home/resources/tga/screen1_16_rle_true.tga",
			"/boot/home/resources/tga/screen1_16.bits", false },
		{ "/boot/home/resources/tga/screen1_24_none_true.tga",
			"/boot/home/resources/tga/screen1_24.bits", false },
		{ "/boot/home/resources/tga/screen1_24_rle_true.tga",
			"/boot/home/resources/tga/screen1_24.bits", false },
		{ "/boot/home/resources/tga/screen1_8_none_cmap.tga",
			"/boot/home/resources/tga/screen1_8_cmap.bits", false },
		{ "/boot/home/resources/tga/screen1_8_none_gray.tga",
			"/boot/home/resources/tga/screen1_8_gray.bits", false },
		{ "/boot/home/resources/tga/screen1_8_rle_cmap.tga",
			"/boot/home/resources/tga/screen1_8_cmap.bits", false },
		{ "/boot/home/resources/tga/grayscreen1_8_rle_cmap.tga",
			"/boot/home/resources/tga/screen1_8_gray.bits", false },
		{ "/boot/home/resources/tga/ugly_16_none_true.tga",
			"/boot/home/resources/tga/ugly_16_none_true.bits", false },
		{ "/boot/home/resources/tga/ugly_24_none_true.tga",
			"/boot/home/resources/tga/ugly_24_none_true.bits", false },
		{ "/boot/home/resources/tga/ugly_32_none_true.tga",
			"/boot/home/resources/tga/ugly_32_none_true.bits", false },
		{ "/boot/home/resources/tga/ugly_8_none_cmap.tga",
			"/boot/home/resources/tga/ugly_8_none_cmap.bits", false }
	};
	const TranslatePaths aBitsInputs[] = {
		{ "/boot/home/resources/tga/cloudcg_24.tga",
			"/boot/home/resources/tga/cloudcg_24.bits", false },
		{ "/boot/home/resources/tga/cloudcg_24_rle.tga",
			"/boot/home/resources/tga/cloudcg_24.bits", true },
		{ "/boot/home/resources/tga/ugly_32.tga",
			"/boot/home/resources/tga/ugly_32_none_true.bits", false },
		{ "/boot/home/resources/tga/ugly_32_rle.tga",
			"/boot/home/resources/tga/ugly_32_none_true.bits", true },
		{ "/boot/home/resources/tga/screen1_24_rle.tga",
			"/boot/home/resources/tga/screen1_24.bits", true },
		{ "/boot/home/resources/tga/ugly_24_rle_true.tga",
			"/boot/home/resources/tga/ugly_24_none_true.bits", true },
		{ "/boot/home/resources/tga/b_gray1-2.tga",
			"/boot/home/resources/tga/b_gray1-2.bits", false },
		{ "/boot/home/resources/tga/b_gray1-2_rle.tga",
			"/boot/home/resources/tga/b_gray1-2.bits", true },
		{ "/boot/home/resources/tga/b_gray1.tga",
			"/boot/home/resources/tga/b_gray1.bits", false },
		{ "/boot/home/resources/tga/b_gray1_rle.tga",
			"/boot/home/resources/tga/b_gray1.bits", true },
		{ "/boot/home/resources/tga/b_rgb15.tga",
			"/boot/home/resources/tga/b_rgb15.bits", false },
		{ "/boot/home/resources/tga/b_rgb15_rle.tga",
			"/boot/home/resources/tga/b_rgb15.bits", true },
		{ "/boot/home/resources/tga/b_rgb16.tga",
			"/boot/home/resources/tga/b_rgb16.bits", false },
		{ "/boot/home/resources/tga/b_rgb16_rle.tga",
			"/boot/home/resources/tga/b_rgb16.bits", true },
		{ "/boot/home/resources/tga/b_rgb32.tga",
			"/boot/home/resources/tga/b_rgb32.bits", false },
		{ "/boot/home/resources/tga/b_rgb32_rle.tga",
			"/boot/home/resources/tga/b_rgb32.bits", true },
		{ "/boot/home/resources/tga/b_cmap8.tga",
			"/boot/home/resources/tga/b_cmap8.bits", false },
		{ "/boot/home/resources/tga/b_cmap8_rle.tga",
			"/boot/home/resources/tga/b_cmap8.bits", true },
	};
	
	TranslateTests(this, proster, aTgaInputs,
		sizeof(aTgaInputs) / sizeof(TranslatePaths), true);
	TranslateTests(this, proster, aBitsInputs,
		sizeof(aBitsInputs) / sizeof(TranslatePaths), false);
	
	delete proster;
	proster = NULL;
}

// Make certain the TGATranslator
// provides a valid configuration message
void
TGATranslatorTest::ConfigMessageTest()
{
	// Init
	NextSubTest();
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/TGATranslator") == B_OK);
		
	// GetAllTranslators
	NextSubTest();
	translator_id tid, *pids = NULL;
	int32 count = 0;
	CPPUNIT_ASSERT(proster->GetAllTranslators(&pids, &count) == B_OK);
	CPPUNIT_ASSERT(pids);
	CPPUNIT_ASSERT(count == 1);
	tid = pids[0];
	delete[] pids;
	pids = NULL;
	
	// GetConfigurationMessage
	NextSubTest();
	BMessage msg;
	CPPUNIT_ASSERT(proster->GetConfigurationMessage(tid, &msg) == B_OK);
	CPPUNIT_ASSERT(!msg.IsEmpty());
	
	// B_TRANSLATOR_EXT_HEADER_ONLY
	NextSubTest();
	bool bheaderonly = true;
	CPPUNIT_ASSERT(
		msg.FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderonly) == B_OK);
	CPPUNIT_ASSERT(bheaderonly == false);
	
	// B_TRANSLATOR_EXT_DATA_ONLY
	NextSubTest();
	bool bdataonly = true;
	CPPUNIT_ASSERT(
		msg.FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataonly) == B_OK);
	CPPUNIT_ASSERT(bdataonly == false);
	
	// "tga /rle"
	NextSubTest();
	bool brle;
	CPPUNIT_ASSERT(msg.FindBool("tga /rle", &brle) == B_OK);
	CPPUNIT_ASSERT(brle == true || brle == false);
	
	// "tga /ignore_alpha"
	NextSubTest();
	bool balpha;
	CPPUNIT_ASSERT(msg.FindBool("tga /ignore_alpha", &balpha) == B_OK);
	CPPUNIT_ASSERT(balpha == true || balpha == false);
}

#if !TEST_R5

// The input formats that this translator is supposed to support
translation_format gTGAInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.7f, // quality
		0.6f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (TGATranslator)"
	},
	{
		B_TGA_FORMAT,
		B_TRANSLATOR_BITMAP,
		0.7f,
		0.8f,
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
		0.7f,
		0.6f,
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
		gTGAOutputFormats, sizeof(gTGAOutputFormats) / sizeof(translation_format),
		B_TRANSLATION_MAKE_VERSION(1,0,0));
}

#endif // #if !TEST_R5
