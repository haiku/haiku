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
#include "../../../../add-ons/translators/bmptranslator/BMPTranslator.h"

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
	suite->addTest(
		new TC("BMPTranslator BTranslatorTranslateTest",
			&BMPTranslatorTest::BTranslatorTranslateTest));
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
	fheader.fileSize = 1028;
	BMallocIO mallabrev;
	CPPUNIT_ASSERT(mallabrev.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallabrev.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(ptran->Identify(&mallabrev, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	// Write out the MS and OS/2 headers with various fields being corrupt, only one 
	// corrupt field at a time, also do abrev test for MS header and OS/2 header
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 53; // bad value, too small to contain all of MS header data
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	MSInfoHeader msheader;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallbadfs;
	CPPUNIT_ASSERT(mallbadfs.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallbadfs.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallbadfs.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallbadfs.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallbadfs.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallbadfs, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 7; // bad value, should be zero
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallbadr;
	CPPUNIT_ASSERT(mallbadr.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallbadr.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallbadr.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallbadr.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallbadr.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallbadr, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 53; // bad value, for MS format, needs to be at least 54
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallbaddo1;
	CPPUNIT_ASSERT(mallbaddo1.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallbaddo1.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo1.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo1.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo1.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallbaddo1, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 25; // bad value, for OS/2 format, needs to be at least 26
	OS2InfoHeader os2header;
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallbaddo2;
	CPPUNIT_ASSERT(mallbaddo2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallbaddo2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo2.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallbaddo2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 1029; // bad value, larger than the fileSize
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallbaddo3;
	CPPUNIT_ASSERT(mallbaddo3.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallbaddo3.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo3.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo3.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallbaddo3.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallbaddo3, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallos2abrev;
	CPPUNIT_ASSERT(mallos2abrev.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2abrev.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2abrev.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2abrev.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2abrev.Write(&os2header, 1) == 1); // only 1 byte of the os2 header included
	CPPUNIT_ASSERT(ptran->Identify(&mallos2abrev, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallos2abrev2;
	CPPUNIT_ASSERT(mallos2abrev2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2abrev2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2abrev2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2abrev2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2abrev2.Write(&os2header, 5) == 5); // most of the os2 header missing
	CPPUNIT_ASSERT(ptran->Identify(&mallos2abrev2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmsabrev1;
	CPPUNIT_ASSERT(mallmsabrev1.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmsabrev1.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmsabrev1.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmsabrev1.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmsabrev1.Write(&msheader, 1) == 1); // only 1 byte of ms header written
	CPPUNIT_ASSERT(ptran->Identify(&mallmsabrev1, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmsabrev2;
	CPPUNIT_ASSERT(mallmsabrev2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmsabrev2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmsabrev2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmsabrev2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmsabrev2.Write(&msheader, 15) == 15); // less than half of ms header written
	CPPUNIT_ASSERT(ptran->Identify(&mallmsabrev2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 39; // size too small for MS format
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmssize;
	CPPUNIT_ASSERT(mallmssize.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmssize.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmssize.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmssize.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmssize.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallmssize, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 41; // size too large for MS format
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmssize2;
	CPPUNIT_ASSERT(mallmssize2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmssize2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmssize2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmssize2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmssize2.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallmssize2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 11; // os2 header size should be 12
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallos2size;
	CPPUNIT_ASSERT(mallos2size.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2size.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2size.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2size.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2size.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallos2size, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 13; // os2 header size should be 12
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallos2size2;
	CPPUNIT_ASSERT(mallos2size2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2size2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2size2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2size2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2size2.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallos2size2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 0; // width of zero is ridiculous
	msheader.height = 5;
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmswidth;
	CPPUNIT_ASSERT(mallmswidth.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmswidth.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmswidth.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmswidth.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmswidth.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallmswidth, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 12;
	os2header.width = 0; // width of zero is ridiculous
	os2header.height = 5;
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallos2width;
	CPPUNIT_ASSERT(mallos2width.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2width.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2width.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2width.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2width.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallos2width, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 0; // zero is not a good value
	msheader.planes = 1;
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmsheight;
	CPPUNIT_ASSERT(mallmsheight.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmsheight.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmsheight.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmsheight.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmsheight.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallmsheight, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 0; // bad value
	os2header.planes = 1;
	os2header.bitsperpixel = 24;
	BMallocIO mallos2height;
	CPPUNIT_ASSERT(mallos2height.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2height.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2height.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2height.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2height.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallos2height, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 0; // should always be 1
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmsplanes;
	CPPUNIT_ASSERT(mallmsplanes.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmsplanes.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmsplanes.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmsplanes.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmsplanes.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallmsplanes, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 54;
	msheader.size = 40;
	msheader.width = 5;
	msheader.height = 5;
	msheader.planes = 2; // should always be 1
	msheader.bitsperpixel = 24;
	msheader.compression = BMP_NO_COMPRESS;
	msheader.imagesize = 0;
	msheader.xpixperm = 23275;
	msheader.ypixperm = 23275;
	msheader.colorsused = 0;
	msheader.colorsimportant = 0;
	BMallocIO mallmsplanes2;
	CPPUNIT_ASSERT(mallmsplanes2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallmsplanes2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallmsplanes2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallmsplanes2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallmsplanes2.Write(&msheader, 40) == 40);
	CPPUNIT_ASSERT(ptran->Identify(&mallmsplanes2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 0; // should always be 1
	os2header.bitsperpixel = 24;
	BMallocIO mallos2planes;
	CPPUNIT_ASSERT(mallos2planes.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2planes.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2planes.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2planes.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2planes.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallos2planes, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 26;
	os2header.size = 12;
	os2header.width = 5;
	os2header.height = 5;
	os2header.planes = 2; // should always be 1
	os2header.bitsperpixel = 24;
	BMallocIO mallos2planes2;
	CPPUNIT_ASSERT(mallos2planes2.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallos2planes2.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(mallos2planes2.Write(&fheader.reserved, 4) == 4);
	CPPUNIT_ASSERT(mallos2planes2.Write(&fheader.dataOffset, 4) == 4);
	CPPUNIT_ASSERT(mallos2planes2.Write(&os2header, 12) == 12);
	CPPUNIT_ASSERT(ptran->Identify(&mallos2planes2, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	
	// makes sure invalid bit depths aren't recognized
	const uint16 bitdepths[] = { 0, 2, 3, 5, 6, 7, 9, 23, 25, 31, 33 };
	const int32 knbitdepths = sizeof(bitdepths) / sizeof(uint16);
	for (int32 i = 0; i < knbitdepths; i++) {
		NextSubTest();
		fheader.magic = 'MB';
		fheader.fileSize = 1028;
		fheader.reserved = 0;
		fheader.dataOffset = 54;
		msheader.size = 40;
		msheader.width = 5;
		msheader.height = 5;
		msheader.planes = 1;
		msheader.bitsperpixel = bitdepths[i];
		msheader.compression = BMP_NO_COMPRESS;
		msheader.imagesize = 0;
		msheader.xpixperm = 23275;
		msheader.ypixperm = 23275;
		msheader.colorsused = 0;
		msheader.colorsimportant = 0;
		BMallocIO mallmsbitdepth;
		mallmsbitdepth.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(mallmsbitdepth.Write(&fheader.magic, 2) == 2);
		CPPUNIT_ASSERT(mallmsbitdepth.Write(&fheader.fileSize, 4) == 4);
		CPPUNIT_ASSERT(mallmsbitdepth.Write(&fheader.reserved, 4) == 4);
		CPPUNIT_ASSERT(mallmsbitdepth.Write(&fheader.dataOffset, 4) == 4);
		CPPUNIT_ASSERT(mallmsbitdepth.Write(&msheader, 40) == 40);
		CPPUNIT_ASSERT(ptran->Identify(&mallmsbitdepth, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
		
		NextSubTest();
		fheader.magic = 'MB';
		fheader.fileSize = 1028;
		fheader.reserved = 0;
		fheader.dataOffset = 26;
		os2header.size = 12;
		os2header.width = 5;
		os2header.height = 5;
		os2header.planes = 1;
		os2header.bitsperpixel = bitdepths[i];
		BMallocIO mallos2bitdepth;
		mallos2bitdepth.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(mallos2bitdepth.Write(&fheader.magic, 2) == 2);
		CPPUNIT_ASSERT(mallos2bitdepth.Write(&fheader.fileSize, 4) == 4);
		CPPUNIT_ASSERT(mallos2bitdepth.Write(&fheader.reserved, 4) == 4);
		CPPUNIT_ASSERT(mallos2bitdepth.Write(&fheader.dataOffset, 4) == 4);
		CPPUNIT_ASSERT(mallos2bitdepth.Write(&os2header, 12) == 12);
		CPPUNIT_ASSERT(ptran->Identify(&mallos2bitdepth, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	}
	
	// makes sure invalid compression values aren't recognized
	const uint16 cbitdepths[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 23, 24, 25, 31, 32, 33 };
	const uint32 compvalues[] = { BMP_RLE4_COMPRESS, BMP_RLE8_COMPRESS, 3, 4, 5, 10 };
	const int32 kncdepths = sizeof(cbitdepths) / sizeof(uint16);
	const int32 kncomps = sizeof(compvalues) / sizeof(uint32);
	for (int32 i = 0; i < kncomps; i++)
		for (int32 n = 0; n < kncdepths; n++) {
			if (!(compvalues[i] == BMP_RLE4_COMPRESS && cbitdepths[n] == 4) &&
				!(compvalues[i] == BMP_RLE8_COMPRESS && cbitdepths[n] == 8)) {
				NextSubTest();
				fheader.magic = 'MB';
				fheader.fileSize = 1028;
				fheader.reserved = 0;
				fheader.dataOffset = 54;
				msheader.size = 40;
				msheader.width = 5;
				msheader.height = 5;
				msheader.planes = 1;
				msheader.bitsperpixel = cbitdepths[n];
				msheader.compression = compvalues[i];
				msheader.imagesize = 0;
				msheader.xpixperm = 23275;
				msheader.ypixperm = 23275;
				msheader.colorsused = 0;
				msheader.colorsimportant = 0;
				BMallocIO mallmscomp;
				mallmscomp.Seek(0, SEEK_SET);
				CPPUNIT_ASSERT(mallmscomp.Write(&fheader.magic, 2) == 2);
				CPPUNIT_ASSERT(mallmscomp.Write(&fheader.fileSize, 4) == 4);
				CPPUNIT_ASSERT(mallmscomp.Write(&fheader.reserved, 4) == 4);
				CPPUNIT_ASSERT(mallmscomp.Write(&fheader.dataOffset, 4) == 4);
				CPPUNIT_ASSERT(mallmscomp.Write(&msheader, 40) == 40);
				CPPUNIT_ASSERT(ptran->Identify(&mallmscomp, NULL, NULL, &outinfo, 0)
					== B_NO_TRANSLATOR);
			}
		}
		
	// too many colorsused test!
	const uint16 colordepths[] = { 1, 4, 8, 24, 32 };
	const int32 kncolordepths = sizeof(colordepths) / sizeof(uint16);
	for (int32 i = 0; i < kncolordepths; i++) {
		NextSubTest();
		fheader.magic = 'BM';
		fheader.fileSize = 1028;
		fheader.reserved = 0;
		fheader.dataOffset = 54;
		msheader.size = 40;
		msheader.width = 5;
		msheader.height = 5;
		msheader.planes = 1;
		msheader.bitsperpixel = colordepths[i];
		msheader.compression = BMP_NO_COMPRESS;
		msheader.imagesize = 0;
		msheader.xpixperm = 23275;
		msheader.ypixperm = 23275;
		msheader.colorsused = 0; //(1 << colordepths[i])/* + 1*/;
		msheader.colorsimportant = 0; //(1 << colordepths[i])/* + 1*/;
		BMallocIO mallmscolors;
		mallmscolors.Seek(0, SEEK_SET);
		CPPUNIT_ASSERT(mallmscolors.Write(&fheader.magic, 2) == 2);
		CPPUNIT_ASSERT(mallmscolors.Write(&fheader.fileSize, 4) == 4);
		CPPUNIT_ASSERT(mallmscolors.Write(&fheader.reserved, 4) == 4);
		CPPUNIT_ASSERT(mallmscolors.Write(&fheader.dataOffset, 4) == 4);
		CPPUNIT_ASSERT(mallmscolors.Write(&msheader, 40) == 40);
		CPPUNIT_ASSERT(ptran->Identify(&mallmscolors, NULL, NULL, &outinfo, 0) == B_NO_TRANSLATOR);
	}
	
	// . Release should return NULL because Release has been called
	// as many times as it has been acquired
	NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
	
	// . Unload Add-on
	NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK); 
}

bool
AreIdentical(BPositionIO *pA, BPositionIO *pB)
{
	if (!pA || !pB)
		return false;
	
	pA->Seek(0, SEEK_SET);
	pB->Seek(0, SEEK_SET);
	
	const int32 kbufsize = 512;
	uint8 bufA[kbufsize], bufB[kbufsize];
	
	ssize_t amtA = pA->Read(bufA, kbufsize);
	ssize_t amtB = pB->Read(bufB, kbufsize);
	while (amtA == amtB && amtA > 0) {
		if (memcmp(bufA, bufB, amtA))
			return false;	
		amtA = pA->Read(bufA, kbufsize);
		amtB = pB->Read(bufB, kbufsize);
	}
	
	if (amtA == amtB)
		return true;
	else {
		printf(" {amtA: %d amtB: %d} ", (int) amtA, (int) amtB);
		return false;
	}
}

void
BMPTranslatorTest::BTranslatorTranslateTest()
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
	
	const char *resourcepath = "/boot/home/Desktop/resources/";
	const char *bmpinimages[] = {
		"blocks_24bit.bmp", "blocks_4bit_rle.bmp", "blocks_8bit_rle.bmp",
		"color_scribbles_24bit.bmp", "color_scribbles_24bit_os2.bmp",
		"color_scribbles_8bit.bmp", "color_scribbles_8bit_os2.bmp", "color_scribbles_8bit_rle.bmp",  
		"color_scribbles_4bit.bmp", "color_scribbles_4bit_os2.bmp", "color_scribbles_4bit_rle.bmp", 
		"color_scribbles_1bit_os2.bmp", "color_scribbles_1bit.bmp",
		"vsmall_24bit.bmp", "vsmall_24bit_os2.bmp",
		"vsmall_8bit.bmp", "vsmall_8bit_os2.bmp", "vsmall_8bit_rle.bmp",
		"vsmall_4bit.bmp", "vsmall_4bit_os2.bmp", "vsmall_4bit_rle.bmp",
		"vsmall_1bit.bmp", "vsmall_1bit_os2.bmp",    
		"gnome_beer_24bit.bmp"
	};
	const int32 knbmpinimages = sizeof(bmpinimages) / sizeof(const char *);
	struct BitsCompare {
		uint32 nbmps;
		const char *bitsimage;
	};
	BitsCompare bitscomps[] = {
		{3, "blocks.bits"},
		{7, "color_scribbles_24bit.bits"},
		{1, "color_scribbles_4bit_rle.bits"},
		{2, "color_scribbles_1bit.bits"},
		{10, "vsmall.bits"},
		{1, "gnome_beer.bits"}
	};
	const int32 knbitscomps = sizeof(bitscomps) / sizeof(BitsCompare);
	// NOTE: this loop is destructive, it destroys the resuability
	// of the bitscomps variable
	int32 iin = 0;
	for (int32 i = 0; i < knbitscomps; i++) {
		char bmppath[512], bitspath[512];
		strcpy(bitspath, resourcepath);
		strcat(bitspath, bitscomps[i].bitsimage);
		printf(" |bits: %s| ", bitspath);
		BFile bitsfile;
		CPPUNIT_ASSERT(bitsfile.SetTo(bitspath, B_READ_ONLY) == B_OK);
		while (bitscomps[i].nbmps > 0 && iin < knbmpinimages) {
			NextSubTest();
			bitscomps[i].nbmps--;
			strcpy(bmppath, resourcepath);
			strcat(bmppath, bmpinimages[iin]);
			printf(" |bmp: %s| ", bmppath);
			BFile bmpfile;
			CPPUNIT_ASSERT(bmpfile.SetTo(bmppath, B_READ_ONLY) == B_OK);
			BMallocIO mallio;
			mallio.Seek(0, SEEK_SET);
			CPPUNIT_ASSERT(ptran->Translate(&bmpfile, NULL, NULL, B_TRANSLATOR_BITMAP, &mallio) == B_OK);
			bool bresult = AreIdentical(&bitsfile, &mallio);
			if (!bresult) {
				BFile dbgout("/boot/home/trantest.bits", B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
				dbgout.Write(mallio.Buffer(), mallio.BufferLength());				
			}
			CPPUNIT_ASSERT(bresult);
			iin++;
		}
	}
	
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
	const char *resourcepath = "/boot/home/Desktop/resources/";
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
