/*****************************************************************************/
// OpenBeOS Translation Kit Test
// Author: Brian Matzon <brian@matzon.dk>
// Version: 0.1.0
//
// This is the Test application for BBitmapStream
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
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
#include <stdio.h>
#include <TranslatorRoster.h>
#include <Application.h>
#include <Bitmap.h>

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
		
	/* add suckers */
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::Initialize Test", &BitmapStreamTest::InitializeTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::Constructor Test", &BitmapStreamTest::ConstructorTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::DetachBitmap Test", &BitmapStreamTest::DetachBitmapTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::Position Test", &BitmapStreamTest::PositionTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::ReadAt Test", &BitmapStreamTest::ReadAtTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::Seek Test", &BitmapStreamTest::SeekTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::SetSize Test", &BitmapStreamTest::SetSizeTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::WriteAt Test", &BitmapStreamTest::WriteAtTest));
	suite->addTest(new CppUnit::TestCaller<BitmapStreamTest>("BitmapStreamTest::Size Test", &BitmapStreamTest::SizeTest));					

	return suite;
}

/**
 * Initializes the test
 */
void
BitmapStreamTest::InitializeTest()
{
}

/**
 * Tests:
 * BBitmapStream(BBitmap *map = NULL)
 */
void
BitmapStreamTest::ConstructorTest()
{
	//BBitmapStream with a blank bitmap
	NextSubTest();
	BBitmapStream streamEmpty;
	BBitmap *pbits = NULL;
	CPPUNIT_ASSERT(streamEmpty.Position() == 0);
	CPPUNIT_ASSERT(streamEmpty.Size() == 0);
	CPPUNIT_ASSERT(streamEmpty.DetachBitmap(&pbits) == B_ERROR);
	CPPUNIT_ASSERT(pbits == NULL);
	
	//constructor is created with a value in DetachBitmapTest()
}

/**
 * Tests:
 * status_t DetachBitmap(BBitmap **outMap)
 */
void
BitmapStreamTest::DetachBitmapTest()
{
	BApplication
		app("application/x-vnd.OpenBeOS-translationkit_bitmapstreamtest");
	
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
	
	//make sure that deleting the stream
	//does not destroy the detached BBitmap
	NextSubTest();
	delete pstream;
	pstream = NULL;
	CPPUNIT_ASSERT(pbits->IsValid());
	
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
 * off_t Position() const
 */
void
BitmapStreamTest::PositionTest()
{
}

/**
 * Tests:
 * ssize_t ReadAt(off_t pos, void *buffer, size_t *size)
 */
void
BitmapStreamTest::ReadAtTest()
{
}

/**
 * Tests:
 * off_t Seek(off_t position, uint32 whence)
 */
void
BitmapStreamTest::SeekTest()
{
}

/**
 * Tests:
 * status_t SetSize(off_t size) const
 */
void
BitmapStreamTest::SetSizeTest()
{
}

/**
 * Tests:
 * ssize_t WriteAt(off_t pos, const void *data, size_t *size)
 */
void
BitmapStreamTest::WriteAtTest()
{
}

/**
 * Tests:
 * off_t Size() const
 */
void
BitmapStreamTest::SizeTest()
{
}
