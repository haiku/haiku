/*****************************************************************************/
// OpenBeOS Translation Kit Test
// Author: Michael Wilber
// Version:
//
// This is the Test application for BTranslationUtils
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
#include "TranslationUtilsTest.h"
#include <stdio.h>
#include <TranslatorFormats.h>
	// for B_TRANSLATOR_EXT_*
#include <TranslatorRoster.h>
#include <Application.h>
#include <Bitmap.h>
#include <OS.h>

/* cppunit framework */
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

/**
 * Default constructor - no work
 */
TranslationUtilsTest::TranslationUtilsTest(std::string name)
	: BTestCase(name)
{
}

/**
 * Default destructor - no work
 */
TranslationUtilsTest::~TranslationUtilsTest()
{
}

CppUnit::Test *
TranslationUtilsTest::Suite()
{
	/* create our suite */
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("TranslationUtils");
		
	/* add suckers */
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::GetBitmap Test", &TranslationUtilsTest::GetBitmapTest));
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::GetStyledText Test", &TranslationUtilsTest::GetStyledTextTest));
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::PutStyledText Test", &TranslationUtilsTest::PutStyledTextTest));
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::GetDefaultSettings Test", &TranslationUtilsTest::GetDefaultSettingsTest));
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::AddTranslationItems Test", &TranslationUtilsTest::AddTranslationItemsTest));

	return suite;
}

void
TranslationUtilsTest::GetBitmapTest()
{
	// Open a PNG image file using GetBitmap without specifying
	// a BTranslatorRoster
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translationutilstest");
	BBitmap *pbits = NULL;
	pbits = BTranslationUtils::GetBitmap(
		"../src/tests/kits/translation/data/images/image.png");
	CPPUNIT_ASSERT(pbits);
	CPPUNIT_ASSERT(pbits->Bits());
	CPPUNIT_ASSERT(pbits->BitsLength() == 443904);
	CPPUNIT_ASSERT(pbits->BytesPerRow() == 1536);
	CPPUNIT_ASSERT(pbits->Bounds().IntegerWidth() == 383);
	CPPUNIT_ASSERT(pbits->Bounds().IntegerHeight() == 288);
	delete pbits;
	pbits = NULL;
}

void
TranslationUtilsTest::GetStyledTextTest()
{
	NextSubTest();
}

void
TranslationUtilsTest::PutStyledTextTest()
{
	NextSubTest();
}

void
TranslationUtilsTest::GetDefaultSettingsTest()
{
	// Test translator_id version with a BTranslatorRoster * supplied
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translationutilstest");
	BMessage *pmsg = NULL;
	bool bdummy = false;
	translator_id *pids = NULL;
	int32 ncount = 0;
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->GetAllTranslators(&pids, &ncount) == B_OK);
	CPPUNIT_ASSERT(pids && ncount > 0);
	pmsg = BTranslationUtils::GetDefaultSettings(pids[0], proster);
	CPPUNIT_ASSERT(pmsg);
	delete pmsg;
	pmsg = NULL;
	
	// Test translator_id version without a BTranslatorRoster supplied
	NextSubTest();
	pmsg = BTranslationUtils::GetDefaultSettings(pids[0]);
	CPPUNIT_ASSERT(pmsg);
	delete pmsg;
	pmsg = NULL;
	delete[] pids;
	pids = NULL;
	
	// Get settings from the OBOS TGATranslator and ensure that
	// all of its settings are there
	NextSubTest();
	pmsg = BTranslationUtils::GetDefaultSettings("TGA Images", 100);
	CPPUNIT_ASSERT(pmsg);
	CPPUNIT_ASSERT(pmsg->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY,
		&bdummy) == B_OK);
	CPPUNIT_ASSERT(pmsg->FindBool(B_TRANSLATOR_EXT_DATA_ONLY,
		&bdummy) == B_OK);
	CPPUNIT_ASSERT(pmsg->FindBool("tga /rle", &bdummy) == B_OK);
	delete pmsg;
	pmsg = NULL;
}

void
TranslationUtilsTest::AddTranslationItemsTest()
{
	NextSubTest();
}

