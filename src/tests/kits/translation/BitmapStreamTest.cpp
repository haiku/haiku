/*****************************************************************************/
// Haiku Translation Kit Test
// Authors: Brian Matzon <brian@matzon.dk>, Michael Wilber
// Version:
//
// This is the Test application for BBitmapStream
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 Haiku Project
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
#include "BitmapStreamTest.h"

#include <TranslatorRoster.h>
#include <Application.h>
#include <Bitmap.h>

#include <stdio.h>
#include <string.h>

/* cppunit framework */
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

/**
 * Default constructor - no work
 */
BitmapStreamTest::BitmapStreamTest(std::string name)
	: BTestCase(name)
{
}

/**
 * Default destructor - no work
 */
BitmapStreamTest::~BitmapStreamTest()
{
}

CppUnit::Test *
BitmapStreamTest::Suite()
{
	/* create our suite */
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("BitmapStream");
	typedef CppUnit::TestCaller<BitmapStreamTest> TC;
		
	/* add tests */
	suite->addTest(new TC("BitmapStreamTest::Constructor Test",
		&BitmapStreamTest::ConstructorTest));
		
	suite->addTest(new TC("BitmapStreamTest::DetachBitmap Test",
		&BitmapStreamTest::DetachBitmapTest));
		
	suite->addTest(new TC("BitmapStreamTest::Seek Test",
		&BitmapStreamTest::SeekTest));
		
	suite->addTest(new TC("BitmapStreamTest::SetSize Test",
		&BitmapStreamTest::SetSizeTest));
		
	suite->addTest(new TC("BitmapStreamTest::ReadWrite Test",
		&BitmapStreamTest::ReadWriteTest));

	return suite;
}

/**
 * Tests:
 * BBitmapStream(BBitmap *map = NULL)
 */
void
BitmapStreamTest::ConstructorTest()
{
	BApplication
		app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

	//BBitmapStream with no bitmap supplied
	NextSubTest();
	BBitmapStream streamEmpty;
	BBitmap *pbits = NULL;
	CPPUNIT_ASSERT(streamEmpty.Position() == 0);
	CPPUNIT_ASSERT(streamEmpty.Size() == 0);
	CPPUNIT_ASSERT(streamEmpty.DetachBitmap(&pbits) == B_ERROR);
	CPPUNIT_ASSERT(pbits == NULL);
	
	//BBitmapStream with an empty BBitmap
	NextSubTest();
	pbits = new BBitmap(BRect(0,0,5,5), B_RGB32);
	CPPUNIT_ASSERT(pbits);
	
	BBitmapStream *pstreamWithBits;
	pstreamWithBits = new BBitmapStream(pbits);
	CPPUNIT_ASSERT(pstreamWithBits);
	CPPUNIT_ASSERT(pstreamWithBits->Position() == 0);
	CPPUNIT_ASSERT(pstreamWithBits->Size() == 176);
	BBitmap *poutbits = NULL;
	CPPUNIT_ASSERT(pstreamWithBits->DetachBitmap(&poutbits) == B_OK);
	CPPUNIT_ASSERT(pbits == poutbits);
	
	delete pstreamWithBits;
	pstreamWithBits = NULL;
	
	delete pbits;
	pbits = NULL;
	
	//constructor is also created with a value in DetachBitmapTest()
}

/**
 * Tests:
 * status_t DetachBitmap(BBitmap **outMap)
 */
void
BitmapStreamTest::DetachBitmapTest()
{
	BApplication
		app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
	
	NextSubTest();
	BFile file("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(file.InitCheck() == B_OK);

	//translate a file into a BBitmapStream
	NextSubTest();
	BBitmapStream *pstream = new BBitmapStream;
	BBitmap *pbits = NULL;
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster->Translate(&file, NULL, NULL, pstream,
		B_TRANSLATOR_BITMAP) == B_OK);
	CPPUNIT_ASSERT(pstream->DetachBitmap(&pbits) == B_NO_ERROR);
	CPPUNIT_ASSERT(pbits);
	CPPUNIT_ASSERT(pbits->IsValid());
	
	//B_BAD_VALUE
	NextSubTest();
	CPPUNIT_ASSERT(pstream->DetachBitmap(NULL) == B_BAD_VALUE);
	
	//Attempt double detach
	NextSubTest();
	BBitmap *pdbits = NULL;
	CPPUNIT_ASSERT(pstream->DetachBitmap(&pdbits) == B_ERROR);
	
	//make sure that deleting the stream
	//does not destroy the detached BBitmap
	NextSubTest();
	delete pstream;
	pstream = NULL;
	CPPUNIT_ASSERT(pbits->IsValid());
	CPPUNIT_ASSERT(pbits->BitsLength() > 0);
	
	//create a new stream using the BBitmap
	//created by the first stream
	NextSubTest();
	BBitmapStream *pfullstream = new BBitmapStream(pbits);
	CPPUNIT_ASSERT(pfullstream->Position() == 0);
	CPPUNIT_ASSERT(pfullstream->Size() ==
		pbits->BitsLength() + sizeof(TranslatorBitmap));
		
	//deleting pfullstream should also destroy
	//the bitmap attached to it (pbits)
	NextSubTest();
	delete pfullstream;
	pfullstream = NULL;
	CPPUNIT_ASSERT(pbits->BitsLength() == 0);
}

/**
 * Tests:
 * ssize_t ReadAt(off_t pos, void *buffer, size_t numBytes)
 * ssize_t WriteAt(off_t pos, void *buffer, size_t numBytes)
 */
void
BitmapStreamTest::ReadWriteTest()
{
	NextSubTest();
	BApplication
		app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
	char chbuf[sizeof(TranslatorBitmap)],
		chheader[sizeof(TranslatorBitmap)], *pch;
	TranslatorBitmap sheader;
	BBitmapStream stream;
	int32 width = 5, height = 5;
	
	sheader.magic = B_TRANSLATOR_BITMAP;
	sheader.bounds.left = 0;
	sheader.bounds.top = 0;
	sheader.bounds.right = width - 1;
	sheader.bounds.bottom = height - 1;
	sheader.rowBytes = width * 4;
	sheader.colors = B_RGB32;
	sheader.dataSize = sheader.rowBytes * height;
	
	memcpy(&chheader, &sheader, sizeof(TranslatorBitmap));
	CPPUNIT_ASSERT(swap_data(B_UINT32_TYPE, &(chheader[0]),
		sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) == B_OK);
		
	// Write header, 1 byte at a time
	NextSubTest();
	off_t nPos;
	for (nPos = 0; nPos < sizeof(TranslatorBitmap); nPos++) {
		pch = (char *)(&(chheader[0])) + nPos;
		CPPUNIT_ASSERT(stream.WriteAt(nPos, pch, 1) == 1);
	}
	// Check size
	NextSubTest();
	CPPUNIT_ASSERT(stream.Size() ==
		sizeof(TranslatorBitmap) + sheader.dataSize);
		
	// Read header, 1 byte at a time
	NextSubTest();
	for (nPos = 0; nPos < sizeof(TranslatorBitmap); nPos++) {
		pch = (char *)(&(chheader[0])) + nPos;
		CPPUNIT_ASSERT(stream.ReadAt(nPos, &(chbuf[0]), 1) == 1);
		CPPUNIT_ASSERT(pch[0] == chbuf[0]);
	}
	
	// Write header, all at once
	NextSubTest();
	pch = (char *)(&(chheader[0]));
	CPPUNIT_ASSERT(stream.WriteAt(0, pch,
		sizeof(TranslatorBitmap)) == sizeof(TranslatorBitmap));
	// Check size
	NextSubTest();
	CPPUNIT_ASSERT(stream.Size() ==
		sizeof(TranslatorBitmap) + sheader.dataSize);
		
	// Read header, all at once
	NextSubTest();
	CPPUNIT_ASSERT(stream.ReadAt(0, &(chbuf[0]),
		sizeof(TranslatorBitmap)) == sizeof(TranslatorBitmap));
	CPPUNIT_ASSERT(memcmp(pch, &(chbuf[0]), sizeof(TranslatorBitmap)) == 0);
	
	// Write bitmap data
	NextSubTest();
	int32 bytesLeft = sheader.dataSize;
	char byt = 0xCF;
	nPos = sizeof(TranslatorBitmap);
	while (bytesLeft--)
		CPPUNIT_ASSERT(stream.WriteAt(nPos++, &byt, 1) == 1);
	// Check size
	NextSubTest();
	CPPUNIT_ASSERT(stream.Size() ==
		sizeof(TranslatorBitmap) + sheader.dataSize);
		
	// Test reading zero bytes
	NextSubTest();
	CPPUNIT_ASSERT(stream.ReadAt(stream.Size(), &(chbuf[0]), 0) == 0);
	CPPUNIT_ASSERT(stream.ReadAt(sheader.dataSize + 1000, &(chbuf[0]), 0) == 0);
	CPPUNIT_ASSERT(stream.ReadAt(-1, &(chbuf[0]), 0) == 0);
	
	// Read bitmap data
	NextSubTest();
	#if !TEST_R5
		// This test fails with Be's version because of a bug.
		// Be's BBitmapStream::ReadAt() has strange behavior in cases
		// where the pos parameter of ReadAt() is != BBitmapStream::Position().
		// If BBitmapStream::Read() is used instead, it calls 
		// BBitmapStream::ReadAt() with pos = BBitmapStream::Position(),
		// so, this issue is rarely a problem because Read() is most often used.
		bytesLeft = sheader.dataSize;
		nPos = sizeof(TranslatorBitmap);
		while (bytesLeft--) {
			chbuf[0] = 0x99;
			ssize_t rd = stream.ReadAt(nPos++, &(chbuf[0]), 1);
			CPPUNIT_ASSERT(rd == 1);
			CPPUNIT_ASSERT(chbuf[0] == byt);
		}
	#endif
	
	// Send erroneous and weird data to WriteAt()
	NextSubTest();
	CPPUNIT_ASSERT(stream.WriteAt(0, &byt, 0) == 0);
	CPPUNIT_ASSERT(stream.WriteAt(-1, &byt, 1) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.WriteAt(stream.Size(), &byt, 1) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.WriteAt(stream.Size() + 1, &byt, 1) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.WriteAt(0, NULL, 1) == B_BAD_VALUE);
	
	// Send erroneous and weird data to ReadAt()
	NextSubTest();
	CPPUNIT_ASSERT(stream.ReadAt(0, &(chbuf[0]), 0) == 0);
	CPPUNIT_ASSERT(stream.ReadAt(-1, &(chbuf[0]), 1) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.ReadAt(stream.Size(), &(chbuf[0]),
		 1) == B_ERROR);
	CPPUNIT_ASSERT(stream.ReadAt(stream.Size() + 1,
		&(chbuf[0]), 1) == B_ERROR);
	#if !TEST_R5
		// Be's version doesn't check for NULL
		CPPUNIT_ASSERT(stream.ReadAt(0, NULL, 1) == B_BAD_VALUE);
	#endif
	
	// There is a segment violation with Be's version when stream is destroyed.
	// Don't yet know why.
}

/**
 * Tests:
 * off_t Seek(off_t position, uint32 whence)
 * off_t Position() const
 */
void
BitmapStreamTest::SeekTest()
{
	BApplication
		app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
	
	NextSubTest();
	BFile file("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(file.InitCheck() == B_OK);

	//translate a file into a BBitmapStream
	NextSubTest();
	BBitmapStream stream;
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster->Translate(&file, NULL, NULL, &stream,
		B_TRANSLATOR_BITMAP) == B_OK);
	
	// Test SEEK_END
	NextSubTest();
	off_t nPos;
	nPos = stream.Size();
	CPPUNIT_ASSERT(stream.Seek(0, SEEK_END) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	nPos = 0;
	CPPUNIT_ASSERT(stream.Seek(-stream.Size(), SEEK_END) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	nPos = stream.Size() - 15;
	CPPUNIT_ASSERT(stream.Seek(-15, SEEK_END) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	CPPUNIT_ASSERT(stream.Seek(1, SEEK_END) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	CPPUNIT_ASSERT(stream.Seek(-(stream.Size() + 1),
		SEEK_END) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	// Test SEEK_SET
	NextSubTest();
	nPos = 0;
	CPPUNIT_ASSERT(stream.Seek(0, SEEK_SET) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	nPos = stream.Size();
	CPPUNIT_ASSERT(stream.Seek(nPos, SEEK_SET) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	nPos /= 2;
	CPPUNIT_ASSERT(stream.Seek(nPos, SEEK_SET) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	CPPUNIT_ASSERT(stream.Seek(-1, SEEK_SET) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	CPPUNIT_ASSERT(stream.Seek(stream.Size() + 1, SEEK_SET) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	// Test SEEK_CUR
	NextSubTest();
	CPPUNIT_ASSERT(stream.Seek(-nPos, SEEK_CUR) == 0);
	CPPUNIT_ASSERT(stream.Position() == 0);
	
	nPos = stream.Size();
	CPPUNIT_ASSERT(stream.Seek(nPos, SEEK_CUR) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	nPos -= 11;
	CPPUNIT_ASSERT(stream.Seek(-11, SEEK_CUR) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	nPos += 8;
	CPPUNIT_ASSERT(stream.Seek(8, SEEK_CUR) == nPos);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	CPPUNIT_ASSERT(stream.Seek(-(stream.Position() + 1),
		SEEK_CUR) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.Position() == nPos);
	
	CPPUNIT_ASSERT(stream.Seek((stream.Size() - stream.Position()) + 1,
		SEEK_CUR) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.Position() == nPos);	
}

/**
 * Tests:
 * status_t SetSize(off_t size) const
 */
void
BitmapStreamTest::SetSizeTest()
{
	BApplication
		app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
	
	NextSubTest();
	BFile file("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(file.InitCheck() == B_OK);

	//translate a file into a BBitmapStream
	NextSubTest();
	BBitmapStream stream;
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster->Translate(&file, NULL, NULL, &stream,
		B_TRANSLATOR_BITMAP) == B_OK);
	
	// Send crap to SetSize
	NextSubTest();
	CPPUNIT_ASSERT(stream.SetSize(-1) == B_BAD_VALUE);
	CPPUNIT_ASSERT(stream.SetSize(stream.Size() + 1) == B_BAD_VALUE);
}
