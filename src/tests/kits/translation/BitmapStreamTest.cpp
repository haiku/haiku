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
BitmapStreamTest::BitmapStreamTest(std::string name) : BTestCase(name) {
}

/**
 * Default destructor - no work
 */
BitmapStreamTest::~BitmapStreamTest() {
}

CppUnit::Test*
BitmapStreamTest::Suite() {
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
void BitmapStreamTest::InitializeTest() {
	//aquire default roster
	NextSubTest();
	roster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(roster != NULL);
}

/**
 * Tests:
 * BBitmapStream(BBitmap *map = NULL)
 */
void BitmapStreamTest::ConstructorTest() {
}

/**
 * Tests:
 * status_t DetachBitmap(BBitmap **outMap)
 */
void BitmapStreamTest::DetachBitmapTest() {
	BApplication app("application/x-vnd.OpenBeOS-translationkit_bitmapstreamtest");
	
	NextSubTest();
	BFile file("../src/tests/kits/translation/data/images/image.jpg", B_READ_ONLY);
	CPPUNIT_ASSERT(file.InitCheck() == B_OK);

	NextSubTest();
	BBitmapStream stream;
	BBitmap* result = NULL;
	roster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(roster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP) == B_OK);
	CPPUNIT_ASSERT(stream.DetachBitmap(&result) == B_NO_ERROR);
	
	CPPUNIT_ASSERT(result != NULL);
	if(result != NULL) {
		delete result;
	}
}

/**
 * Tests:
 * off_t Position() const
 */
void BitmapStreamTest::PositionTest() {
}

/**
 * Tests:
 * ssize_t ReadAt(off_t pos, void *buffer, size_t *size)
 */
void BitmapStreamTest::ReadAtTest() {
}

/**
 * Tests:
 * off_t Seek(off_t position, uint32 whence)
 */
void BitmapStreamTest::SeekTest() {
}

/**
 * Tests:
 * status_t SetSize(off_t size) const
 */
void BitmapStreamTest::SetSizeTest() {
}

/**
 * Tests:
 * ssize_t WriteAt(off_t pos, const void *data, size_t *size)
 */
void BitmapStreamTest::WriteAtTest() {
}

/**
 * Tests:
 * off_t Size() const
 */
void BitmapStreamTest::SizeTest() {
}
