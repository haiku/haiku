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
#include <File.h>
#include <DataIO.h>
#include <BMPTranslator.h>

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
		new TC("BMPTranslator BTranslatorBasicTest",
			&BMPTranslatorTest::BTranslatorBasicTest));
	suite->addTest(
		new TC("BMPTranslator BTranslatorIdentifyErrorTest",
			&BMPTranslatorTest::BTranslatorIdentifyErrorTest));
	suite->addTest(
		new TC("BMPTranslator BTranslatorIdentifyTest",
			&BMPTranslatorTest::BTranslatorIdentifyTest));
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
BMPTranslatorTest::BTranslatorBasicTest()
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
	
	NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);
	
	NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);
	
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == ptran);
	
	NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);
	
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
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, BITS_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pins[i].name,
					"Be Bitmap Format (BMPTranslator)") == 0);
			} else if (pins[i].type == B_BMP_FORMAT) {
				CPPUNIT_ASSERT(strcmp(pins[i].MIME, BMP_MIME_STRING) == 0);
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
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, BITS_MIME_STRING) == 0);
				CPPUNIT_ASSERT(strcmp(pouts[i].name,
					"Be Bitmap Format (BMPTranslator)") == 0);
			} else if (pouts[i].type == B_BMP_FORMAT) {
				CPPUNIT_ASSERT(strcmp(pouts[i].MIME, BMP_MIME_STRING) == 0);
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

void
BMPTranslatorTest::BTranslatorIdentifyErrorTest()
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
	
	// empty
	NextSubTest();
	translator_info outinfo;
	BMallocIO mallempty;
	CPPUNIT_ASSERT(ptran->Identify(&mallempty, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	// weird, non-image data
	NextSubTest();
	const char *strmonkey = "monkey monkey monkey";
	BMemoryIO memmonkey(strmonkey, strlen(strmonkey));
	CPPUNIT_ASSERT(ptran->Identify(&memmonkey, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	// abreviated BMPFileHeader
	NextSubTest();
	BMPFileHeader fheader;
	fheader.magic = 'MB';
	fheader.fileSize = 128;
	BMallocIO mallabrev;
	CPPUNIT_ASSERT(mallabrev.Write(&fheader.magic, sizeof(uint16)) == sizeof(uint16));
	CPPUNIT_ASSERT(mallabrev.Write(&fheader.fileSize, sizeof(uint32)) == sizeof(uint32));
	CPPUNIT_ASSERT(ptran->Identify(&mallabrev, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	// Write out the MS and OS/2 headers with various fields being corrupt, only one 
	// corrupt field at a time, also do abrev test for MS header and OS/2 header	
	
	// . Release should return NULL because Release has been called
	// as many times as it has been acquired
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
	
	// . Unload Add-on
	NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK); 
}

void
test_outinfo(translator_info &info, bool isbmp)
{
	if (isbmp) {
		CPPUNIT_ASSERT(info.type == B_BMP_FORMAT);
		CPPUNIT_ASSERT(strcmp(info.MIME, BMP_MIME_STRING) == 0);
	} else {
		CPPUNIT_ASSERT(info.type == B_TRANSLATOR_BITMAP);
		CPPUNIT_ASSERT(strcmp(info.MIME, BITS_MIME_STRING) == 0);
	}
	CPPUNIT_ASSERT(info.name);
	CPPUNIT_ASSERT(info.group == B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(info.quality > 0 && info.quality <= 1);
	CPPUNIT_ASSERT(info.capability > 0 && info.capability <= 1);
}

void
BMPTranslatorTest::BTranslatorIdentifyTest()
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
	
	// . call identify on a BMP with various different options
	const char *resourcepath =
		"../src/tests/add-ons/translators/bmptranslator/resources/";
	const char *testimages[] = {
		"blocks.bits", "color_scribbles_8bit_os2.bmp",
		"blocks_24bit.bmp", "color_scribbles_8bit_rle.bmp",
		"blocks_4bit_rle.bmp", "gnome_beer.bits",
		"blocks_8bit_rle.bmp", "gnome_beer_24bit.bmp",
		"color_scribbles_1bit.bits", "vsmall.bits",
		"color_scribbles_1bit.bmp", "vsmall_1bit.bmp",
		"color_scribbles_1bit_os2.bmp", "vsmall_1bit_os2.bmp",
		"color_scribbles_24bit.bits", "vsmall_24bit.bmp",
		"color_scribbles_24bit.bmp", "vsmall_24bit_os2.bmp",
		"color_scribbles_24bit_os2.bmp", "vsmall_4bit.bmp",
		"color_scribbles_4bit.bmp", "vsmall_4bit_os2.bmp",
		"color_scribbles_4bit_os2.bmp", "vsmall_4bit_rle.bmp",
		"color_scribbles_4bit_rle.bits", "vsmall_8bit.bmp",
		"color_scribbles_4bit_rle.bmp", "vsmall_8bit_os2.bmp",
		"color_scribbles_8bit.bmp", "vsmall_8bit_rle.bmp"
	};
	char imagepath[512] = { 0 };
	const int32 knimages = 30;
	BMessage emptymsg;
	translation_format hintformat;
	translator_info outinfo;
	
	// NOTE: This code assumes that BMP files end with ".bmp" and
	// Be Bitmap Images end with ".bits", if this is not true, the
	// test will not work properly
	for (int32 i = 0; i < knimages; i++) {
		NextSubTest();
		strcpy(imagepath, resourcepath);
		strcat(imagepath, testimages[i]);
		int32 pathlen = strlen(imagepath);
		bool isbmp;
		if (imagepath[pathlen - 1] == 'p')
			isbmp = true;
		else
			isbmp = false;
		printf(" [%s] ", testimages[i]);
		BFile imagefile;
		CPPUNIT_ASSERT(imagefile.SetTo(imagepath, B_READ_ONLY) == B_OK);
		
		////////// No hints or io extension
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, NULL, &outinfo, 0) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, NULL, &outinfo, B_TRANSLATOR_BITMAP) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, NULL, &outinfo, B_BMP_FORMAT) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		///////////// empty io extension
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &emptymsg, &outinfo, 0) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &emptymsg, &outinfo, B_TRANSLATOR_BITMAP) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &emptymsg, &outinfo, B_BMP_FORMAT) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		///////////// with "correct" hint
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		if (isbmp) {
			hintformat.type = B_BMP_FORMAT;
			strcpy(hintformat.MIME, BMP_MIME_STRING);
			strcpy(hintformat.name, "BMP Image");
		} else {
			hintformat.type = B_TRANSLATOR_BITMAP;
			strcpy(hintformat.MIME, BITS_MIME_STRING);
			strcpy(hintformat.name, "Be Bitmap Image");
		}
		hintformat.group = B_TRANSLATOR_BITMAP;
		hintformat.quality = 0.5;
		hintformat.capability = 0.5;
		
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, &hintformat, NULL, &outinfo, 0) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		if (isbmp) {
			hintformat.type = B_BMP_FORMAT;
			strcpy(hintformat.MIME, BMP_MIME_STRING);
			strcpy(hintformat.name, "BMP Image");
		} else {
			hintformat.type = B_TRANSLATOR_BITMAP;
			strcpy(hintformat.MIME, BITS_MIME_STRING);
			strcpy(hintformat.name, "Be Bitmap Image");
		}
		hintformat.group = B_TRANSLATOR_BITMAP;
		hintformat.quality = 0.5;
		hintformat.capability = 0.5;
		
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, &hintformat, NULL, &outinfo, B_TRANSLATOR_BITMAP) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		if (isbmp) {
			hintformat.type = B_BMP_FORMAT;
			strcpy(hintformat.MIME, BMP_MIME_STRING);
			strcpy(hintformat.name, "BMP Image");
		} else {
			hintformat.type = B_TRANSLATOR_BITMAP;
			strcpy(hintformat.MIME, BITS_MIME_STRING);
			strcpy(hintformat.name, "Be Bitmap Image");
		}
		hintformat.group = B_TRANSLATOR_BITMAP;
		hintformat.quality = 0.5;
		hintformat.capability = 0.5;
		
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, &hintformat, NULL, &outinfo, B_BMP_FORMAT) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		///////////// with misleading hint (the hint is probably ignored anyway)
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		hintformat.type = B_WAV_FORMAT;
		strcpy(hintformat.MIME, "audio/wav");
		strcpy(hintformat.name, "WAV Audio");
		hintformat.group = B_TRANSLATOR_SOUND;
		hintformat.quality = 0.5;
		hintformat.capability = 0.5;
		
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, &hintformat, NULL, &outinfo, 0) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		hintformat.type = B_WAV_FORMAT;
		strcpy(hintformat.MIME, "audio/wav");
		strcpy(hintformat.name, "WAV Audio");
		hintformat.group = B_TRANSLATOR_SOUND;
		hintformat.quality = 0.5;
		hintformat.capability = 0.5;
		
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, &hintformat, NULL, &outinfo, B_TRANSLATOR_BITMAP) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		hintformat.type = B_WAV_FORMAT;
		strcpy(hintformat.MIME, "audio/wav");
		strcpy(hintformat.name, "WAV Audio");
		hintformat.group = B_TRANSLATOR_SOUND;
		hintformat.quality = 0.5;
		hintformat.capability = 0.5;
		
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, &hintformat, NULL, &outinfo, B_BMP_FORMAT) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		// ioExtension Tests
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		BMessage msgheaderonly;
		msgheaderonly.AddBool(B_TRANSLATOR_EXT_HEADER_ONLY, true);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgheaderonly, &outinfo, 0) == B_OK);
		test_outinfo(outinfo, isbmp);
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgheaderonly, &outinfo, B_BMP_FORMAT) == B_OK);
		test_outinfo(outinfo, isbmp);
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgheaderonly, &outinfo, B_TRANSLATOR_BITMAP) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		BMessage msgdataonly;
		msgdataonly.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, true);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgdataonly, &outinfo, 0) == B_OK);
		test_outinfo(outinfo, isbmp);
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgdataonly, &outinfo, B_BMP_FORMAT) == B_OK);
		test_outinfo(outinfo, isbmp);
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgdataonly, &outinfo, B_TRANSLATOR_BITMAP) == B_OK);
		test_outinfo(outinfo, isbmp);
		
		NextSubTest();
		memset(&outinfo, 0, sizeof(outinfo));
		imagefile.Seek(0, SEEK_SET);
		BMessage msgbothonly;
		msgbothonly.AddBool(B_TRANSLATOR_EXT_HEADER_ONLY, true);
		msgbothonly.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, true);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgbothonly, &outinfo, 0) == B_BAD_VALUE);
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgbothonly, &outinfo, B_BMP_FORMAT) == B_BAD_VALUE);
		imagefile.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(ptran->Identify(&imagefile, NULL, &msgbothonly, &outinfo, B_TRANSLATOR_BITMAP) == B_BAD_VALUE);
	}	
	
	// . Release should return NULL because Release has been called
	// as many times as it has been acquired
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
	
	// . Unload Add-on
	NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK); 
}

#endif // #if !TEST_R5

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
