// BMPTranslatorTest.cpp
#include "BMPTranslatorTest.h"
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

#define BMP_NO_COMPRESS 0
#define BMP_RLE8_COMPRESS 1
#define BMP_RLE4_COMPRESS 2

struct BMPFileHeader {
	// for both MS and OS/2 BMP formats
	uint16 magic;			// = 'BM'
	uint32 fileSize;		// file size in bytes
	uint32 reserved;		// equals 0
	uint32 dataOffset;		// file offset to actual image
};

struct MSInfoHeader {
	uint32 size;			// size of this struct (40)
	uint32 width;			// bitmap width
	uint32 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
	uint32 compression;		// type of compression
	uint32 imagesize;		// size of image data if compressed
	uint32 xpixperm;		// horizontal pixels per meter
	uint32 ypixperm;		// vertical pixels per meter
	uint32 colorsused;		// number of actually used colors
	uint32 colorsimportant;	// number of important colors, zero = all
};

struct OS2InfoHeader {
	uint32 size;			// size of this struct (12)
	uint16 width;			// bitmap width
	uint16 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
};

// Suite
CppUnit::Test *
BMPTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<BMPTranslatorTest> TC;
			
	suite->addTest(
		new TC("BMPTranslator IdentifyTest",
			&BMPTranslatorTest::IdentifyTest));

	suite->addTest(
		new TC("BMPTranslator TranslateTest",
			&BMPTranslatorTest::TranslateTest));
			
	suite->addTest(
		new TC("BMPTranslator ConfigMessageTest",
			&BMPTranslatorTest::ConfigMessageTest));

#if !TEST_R5
	suite->addTest(
		new TC("BMPTranslator LoadAddOnTest",
			&BMPTranslatorTest::LoadAddOnTest));
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

void
CheckBits_Bmp(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP,
		0.4f, 0.6f, "Be Bitmap Format (BMPTranslator)",
		"image/x-be-bitmap");
}

void
CheckBmp(translator_info *pti, const char *imageType)
{
	CheckTranslatorInfo(pti, B_BMP_FORMAT, B_TRANSLATOR_BITMAP,
		0.4f, 0.8f, imageType, "image/x-bmp");
}

// coveniently group path of image with
// the expected Identify() string for that image
struct IdentifyInfo {
	const char *imagePath;
	const char *identifyString;
};

void
IdentifyTests(BMPTranslatorTest *ptest, BTranslatorRoster *proster,
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
			CheckBits_Bmp(&ti);
		else
			CheckBmp(&ti, pinfo[i].identifyString);
	
		// Identify (output: B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_BITMAP) == B_OK);
		if (bbits)
			CheckBits_Bmp(&ti);
		else
			CheckBmp(&ti, pinfo[i].identifyString);
	
		// Identify (output: B_BMP_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_BMP_FORMAT) == B_OK);
		if (bbits)
			CheckBits_Bmp(&ti);
		else
			CheckBmp(&ti, pinfo[i].identifyString);
	}
}

void
BMPTranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/BMPTranslator") == B_OK);
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
	
	// empty
	NextSubTest();
	BMallocIO mallempty;
	CPPUNIT_ASSERT(proster->Identify(&mallempty, NULL, &ti) == B_NO_TRANSLATOR);
	
	// weird, non-image data
	NextSubTest();
	const char *strmonkey = "monkey monkey monkey";
	BMemoryIO memmonkey(strmonkey, strlen(strmonkey));
	CPPUNIT_ASSERT(proster->Identify(&memmonkey, NULL, &ti) == B_NO_TRANSLATOR);
	
	// abreviated BMPFileHeader
	NextSubTest();
	BMPFileHeader fheader;
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	BMallocIO mallabrev;
	CPPUNIT_ASSERT(mallabrev.Write(&fheader.magic, 2) == 2);
	CPPUNIT_ASSERT(mallabrev.Write(&fheader.fileSize, 4) == 4);
	CPPUNIT_ASSERT(proster->Identify(&mallabrev, NULL, &ti) == B_NO_TRANSLATOR);
	
	// Write out the MS and OS/2 headers with various fields being corrupt, only one 
	// corrupt field at a time, also do abrev test for MS header and OS/2 header
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 53; // bad value, too small to contain all of MS header data
		// bad values in this field can be, and are ignored by some Windows image viewers
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
	CPPUNIT_ASSERT(proster->Identify(&mallbadfs, NULL, &ti) == B_OK);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallbadr, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallbaddo1, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallbaddo2, NULL, &ti) == B_NO_TRANSLATOR);
	
	NextSubTest();
	fheader.magic = 'MB';
	fheader.fileSize = 1028;
	fheader.reserved = 0;
	fheader.dataOffset = 1029; // bad value, larger than the fileSize
		// Ignore the fileSize: if it is the case that the actual file size is
		// less than the dataOffset field, the translation will error out appropriately.
		// Assume the fileSize has nothing to do with the actual size of the file
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
	CPPUNIT_ASSERT(proster->Identify(&mallbaddo3, NULL, &ti) == B_OK);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2abrev, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2abrev2, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmsabrev1, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmsabrev2, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmssize, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmssize2, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2size, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2size2, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmswidth, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2width, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmsheight, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2height, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmsplanes, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallmsplanes2, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2planes, NULL, &ti) == B_NO_TRANSLATOR);
	
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
	CPPUNIT_ASSERT(proster->Identify(&mallos2planes2, NULL, &ti) == B_NO_TRANSLATOR);
	
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
		CPPUNIT_ASSERT(proster->Identify(&mallmsbitdepth, NULL, &ti) == B_NO_TRANSLATOR);
		
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
		CPPUNIT_ASSERT(proster->Identify(&mallos2bitdepth, NULL, &ti) == B_NO_TRANSLATOR);
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
				CPPUNIT_ASSERT(proster->Identify(&mallmscomp, NULL, &ti)
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
		CPPUNIT_ASSERT(proster->Identify(&mallmscolors, NULL, &ti) == B_NO_TRANSLATOR);
	}
	
	// Identify (successfully identify the following files)
	const IdentifyInfo aBitsPaths[] = {
		{ "/boot/home/resources/bmp/b_cmap8.bits", "" },
		{ "/boot/home/resources/bmp/b_gray1-2.bits", "" },
		{ "/boot/home/resources/bmp/b_gray1.bits", "" },
		{ "/boot/home/resources/bmp/b_rgb15.bits", "" },
		{ "/boot/home/resources/bmp/b_rgb16.bits", "" },
		{ "/boot/home/resources/bmp/b_rgb32.bits", "" },
		{ "/boot/home/resources/bmp/blocks.bits", "" },
		{ "/boot/home/resources/bmp/gnome_beer.bits", "" },
		{ "/boot/home/resources/bmp/vsmall.bits", "" }
	};
	const IdentifyInfo aBmpPaths[] = {
		{ "/boot/home/resources/bmp/blocks_24bit.bmp",
			"BMP image (MS format, 24 bits)" },
		{ "/boot/home/resources/bmp/blocks_4bit_rle.bmp",
			"BMP image (MS format, 4 bits, RLE)" },
		{ "/boot/home/resources/bmp/blocks_8bit_rle.bmp",
			"BMP image (MS format, 8 bits, RLE)" },
		{ "/boot/home/resources/bmp/color_scribbles_1bit.bmp",
			"BMP image (MS format, 1 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_1bit_os2.bmp",
			"BMP image (OS/2 format, 1 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_24bit.bmp",
			"BMP image (MS format, 24 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_24bit_os2.bmp",
			"BMP image (OS/2 format, 24 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_4bit.bmp",
			"BMP image (MS format, 4 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_4bit_os2.bmp",
			"BMP image (OS/2 format, 4 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_4bit_rle.bmp",
			"BMP image (MS format, 4 bits, RLE)" },
		{ "/boot/home/resources/bmp/color_scribbles_8bit.bmp",
			"BMP image (MS format, 8 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_8bit_os2.bmp",
			"BMP image (OS/2 format, 8 bits)" },
		{ "/boot/home/resources/bmp/color_scribbles_8bit_rle.bmp",
			"BMP image (MS format, 8 bits, RLE)" },
		{ "/boot/home/resources/bmp/gnome_beer_24bit.bmp",
			"BMP image (MS format, 24 bits)" },
		{ "/boot/home/resources/bmp/vsmall_1bit.bmp",
			"BMP image (MS format, 1 bits)" },
		{ "/boot/home/resources/bmp/vsmall_1bit_os2.bmp",
			"BMP image (OS/2 format, 1 bits)" },
		{ "/boot/home/resources/bmp/vsmall_24bit.bmp",
			"BMP image (MS format, 24 bits)" },
		{ "/boot/home/resources/bmp/vsmall_24bit_os2.bmp",
			"BMP image (OS/2 format, 24 bits)" },
		{ "/boot/home/resources/bmp/vsmall_4bit.bmp",
			"BMP image (MS format, 4 bits)" },
		{ "/boot/home/resources/bmp/vsmall_4bit_os2.bmp",
			"BMP image (OS/2 format, 4 bits)" },
		{ "/boot/home/resources/bmp/vsmall_4bit_rle.bmp",
			"BMP image (MS format, 4 bits, RLE)" },
		{ "/boot/home/resources/bmp/vsmall_8bit.bmp",
			"BMP image (MS format, 8 bits)" },
		{ "/boot/home/resources/bmp/vsmall_8bit_os2.bmp",
			"BMP image (OS/2 format, 8 bits)" },
		{ "/boot/home/resources/bmp/vsmall_8bit_rle.bmp",
			"BMP image (MS format, 8 bits, RLE)" },
		{ "/boot/home/resources/bmp/b_rgb32(32).bmp",
			"BMP image (MS format, 32 bits)" },
		{ "/boot/home/resources/bmp/double_click_bmap.bmp",
			"BMP image (MS format, 24 bits)" }
	};

	IdentifyTests(this, proster, aBmpPaths,
		sizeof(aBmpPaths) / sizeof(IdentifyInfo), false);
	IdentifyTests(this, proster, aBitsPaths,
		sizeof(aBitsPaths) / sizeof(IdentifyInfo), true);
	
	delete proster;
	proster = NULL;
}

// coveniently group path of bmp image with
// path of bits image that it should translate to
struct TranslatePaths {
	const char *bmpPath;
	const char *bitsPath;
};

void
TranslateTests(BMPTranslatorTest *ptest, BTranslatorRoster *proster,
	const TranslatePaths *paths, int32 len, bool bbmpinput)
{
	// Perform translations on every file in the array
	for (int32 i = 0; i < len; i++) {
		// Setup input files	
		ptest->NextSubTest();
		BFile bmpfile, bitsfile, *pinput;
		CPPUNIT_ASSERT(bmpfile.SetTo(paths[i].bmpPath, B_READ_ONLY) == B_OK);
		CPPUNIT_ASSERT(bitsfile.SetTo(paths[i].bitsPath, B_READ_ONLY) == B_OK);
		if (bbmpinput) {
			printf(" [%s] ", paths[i].bmpPath);
			pinput = &bmpfile;
		} else {
			printf(" [%s] ", paths[i].bitsPath);
			pinput = &bitsfile;
		}
		
		BMallocIO mallio, dmallio;
		
		// Convert to B_TRANSLATOR_ANY_TYPE (should be B_TRANSLATOR_BITMAP)
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput, NULL, NULL, &mallio,
			B_TRANSLATOR_ANY_TYPE) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bitsfile) == true);
		
		// Convert to B_TRANSLATOR_BITMAP
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput, NULL, NULL, &mallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, bitsfile) == true);
		
		// Convert bits mallio to B_TRANSLATOR_BITMAP dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_BITMAP) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, bitsfile) == true);
		
		// Only perform the following tests if the BMP is not
		// an OS/2 format BMP image.
		// (Need to write special testing for OS/2 images)
		if (!strstr(paths[i].bmpPath, "os2")) {
			// Convert to B_BMP_FORMAT
			ptest->NextSubTest();
			CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
			CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
			CPPUNIT_ASSERT(proster->Translate(pinput, NULL, NULL, &mallio,
				B_BMP_FORMAT) == B_OK);
			CPPUNIT_ASSERT(CompareStreams(mallio, bmpfile) == true);
		
			// Convert BMP mallio to B_TRANSLATOR_BITMAP dmallio
			if (bbmpinput) {
				ptest->NextSubTest();
				CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
				CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
				CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
					B_TRANSLATOR_BITMAP) == B_OK);
				CPPUNIT_ASSERT(CompareStreams(dmallio, bitsfile) == true);
			}
		
			// Convert BMP mallio to B_BMP_FORMAT dmallio
			ptest->NextSubTest();
			CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
			CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
			CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
				B_BMP_FORMAT) == B_OK);
			CPPUNIT_ASSERT(CompareStreams(dmallio, bmpfile) == true);
		}
	}
}

void
BMPTranslatorTest::TranslateTest()
{
	BApplication
		app("application/x-vnd.OpenBeOS-BMPTranslatorTest");
		
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	off_t filesize = -1;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/BMPTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
	BFile output("/tmp/bmp_test.out", B_WRITE_ONLY | 
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
		B_BMP_FORMAT);
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
	
	// For translating BMP images to bits
	const TranslatePaths aBmpInput[] = {
		{ "/boot/home/resources/bmp/blocks_24bit.bmp",
			"/boot/home/resources/bmp/blocks.bits" },
		{ "/boot/home/resources/bmp/blocks_4bit_rle.bmp",
			"/boot/home/resources/bmp/blocks.bits" },
		{ "/boot/home/resources/bmp/blocks_8bit_rle.bmp",
			"/boot/home/resources/bmp/blocks.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_1bit.bmp",
			"/boot/home/resources/bmp/color_scribbles_1bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_1bit_os2.bmp",
			"/boot/home/resources/bmp/color_scribbles_1bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_24bit.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_24bit_os2.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_4bit.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_4bit_os2.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_4bit_rle.bmp",
			"/boot/home/resources/bmp/color_scribbles_4bit_rle.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_8bit.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_8bit_os2.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/color_scribbles_8bit_rle.bmp",
			"/boot/home/resources/bmp/color_scribbles_24bit.bits" },
		{ "/boot/home/resources/bmp/gnome_beer_24bit.bmp",
			"/boot/home/resources/bmp/gnome_beer.bits" },
		{ "/boot/home/resources/bmp/vsmall_1bit.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_1bit_os2.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_24bit.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_24bit_os2.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_4bit.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_4bit_os2.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_4bit_rle.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_8bit.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_8bit_os2.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/vsmall_8bit_rle.bmp",
			"/boot/home/resources/bmp/vsmall.bits" },
		{ "/boot/home/resources/bmp/b_rgb32(32).bmp",
			"/boot/home/resources/bmp/b_rgb32.bits" }
	};
	
	// For translating bits images to BMP
	const TranslatePaths aBitsInput[] = {
		{ "/boot/home/resources/bmp/b_gray1-2.bmp",
			"/boot/home/resources/bmp/b_gray1-2.bits" },
		{ "/boot/home/resources/bmp/b_gray1.bmp",
			"/boot/home/resources/bmp/b_gray1.bits" },
		{ "/boot/home/resources/bmp/b_rgb15.bmp",
			"/boot/home/resources/bmp/b_rgb15.bits" },
		{ "/boot/home/resources/bmp/b_rgb16.bmp",
			"/boot/home/resources/bmp/b_rgb16.bits" },
		{ "/boot/home/resources/bmp/b_rgb32(24).bmp",
			"/boot/home/resources/bmp/b_rgb32.bits" },
		{ "/boot/home/resources/bmp/b_cmap8.bmp",
			"/boot/home/resources/bmp/b_cmap8.bits" }
	};
	
	TranslateTests(this, proster, aBmpInput,
		sizeof(aBmpInput) / sizeof(TranslatePaths), true);
	TranslateTests(this, proster, aBitsInput,
		sizeof(aBitsInput) / sizeof(TranslatePaths), false);
	
	delete proster;
	proster = NULL;
}

// Make certain that the BMPTranslator does not
// provide a configuration message
void
BMPTranslatorTest::ConfigMessageTest()
{
	// Init
	NextSubTest();
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/BMPTranslator") == B_OK);
		
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
}

#if !TEST_R5

// The input formats that this translator is supposed to support
translation_format gBMPInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.4f, // quality
		0.6f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (BMPTranslator)"
	},
	{
		B_BMP_FORMAT,
		B_TRANSLATOR_BITMAP,
		0.4f,
		0.8f,
		"image/x-bmp",
		"BMP image"
	}
};

// The output formats that this translator is supposed to support
translation_format gBMPOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.4f, // quality
		0.6f, // capability
		"image/x-be-bitmap",
		"Be Bitmap Format (BMPTranslator)"
	},
	{
		B_BMP_FORMAT,
		B_TRANSLATOR_BITMAP,
		0.4f,
		0.8f,
		"image/x-bmp",
		"BMP image (MS format)"
	}
};

void
BMPTranslatorTest::LoadAddOnTest()
{
	TranslatorLoadAddOnTest("/boot/home/config/add-ons/Translators/BMPTranslator",
		this,
		gBMPInputFormats, sizeof(gBMPInputFormats) / sizeof(translation_format),
		gBMPOutputFormats, sizeof(gBMPOutputFormats) / sizeof(translation_format),
		B_TRANSLATION_MAKE_VERSION(1,0,0));
}

#endif // #if !TEST_R5
