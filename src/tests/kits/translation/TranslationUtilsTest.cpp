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
#include <TextView.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Entry.h>
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
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::GetPutStyledText Test", &TranslationUtilsTest::GetPutStyledTextTest));
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::GetDefaultSettings Test", &TranslationUtilsTest::GetDefaultSettingsTest));
	suite->addTest(new CppUnit::TestCaller<TranslationUtilsTest>("TranslationUtilsTest::AddTranslationItems Test", &TranslationUtilsTest::AddTranslationItemsTest));

	return suite;
}

void
CheckBitmap(BBitmap *pbits)
{
	CPPUNIT_ASSERT(pbits);
	CPPUNIT_ASSERT(pbits->Bits());
	CPPUNIT_ASSERT(pbits->BitsLength() == 443904);
	CPPUNIT_ASSERT(pbits->BytesPerRow() == 1536);
	CPPUNIT_ASSERT(pbits->Bounds().IntegerWidth() == 383);
	CPPUNIT_ASSERT(pbits->Bounds().IntegerHeight() == 288);
}

void
TranslationUtilsTest::GetBitmapTest()
{
	// File
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translationutilstest");
	BBitmap *pbits = NULL;
	pbits = BTranslationUtils::GetBitmap(
		"../../src/tests/kits/translation/data/images/image.png");
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
	
	// File (passing it a file that isn't actually there)
	pbits = BTranslationUtils::GetBitmap("/tmp/no-file-here.bmp");
	CPPUNIT_ASSERT(pbits == NULL);
	
	// File (GetBitmapFile)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmapFile(
		"../../src/tests/kits/translation/data/images/image.png");
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
	
	// File (passing GetBitmapFile a file that isn't there)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmapFile("/tmp/no-file-here.bmp");
	CPPUNIT_ASSERT(pbits == NULL);
	
	// File (entry_ref)
	NextSubTest();
	entry_ref ref;
	BEntry bent(
		"../src/tests/kits/translation/data/images/image.png");
	CPPUNIT_ASSERT(bent.InitCheck() == B_OK);
	CPPUNIT_ASSERT(bent.GetRef(&ref) == B_OK);
	pbits = BTranslationUtils::GetBitmap(&ref);
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
	
	// File (NULL entry_ref)
	NextSubTest();
	entry_ref *pref = NULL;
	pbits = BTranslationUtils::GetBitmap(pref);
	CPPUNIT_ASSERT(pbits == NULL);
	
	// Resource
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap("res_image");
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
	
	// Resource (bad resource name)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap("Michael Wilber");
	CPPUNIT_ASSERT(pbits == NULL);
	
	// Resource by Type & Id
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap(
		B_TRANSLATOR_BITMAP, 246);
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
	
	// Resource by Type & Id (wrong resource type)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_TEXT, 246);
	CPPUNIT_ASSERT(pbits == NULL);
	
	// Resource by Type & Id (wrong resource Id)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP,
		166);
	CPPUNIT_ASSERT(pbits == NULL);
	
	// Resource by Type & Name
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap(
		B_TRANSLATOR_BITMAP, "res_image");
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
	
	// Resource by Type & Name (wrong resource type)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_TEXT,
		"res_image");
	CPPUNIT_ASSERT(pbits == NULL);
	
	// Resource by Type & Name (wrong resource name)
	NextSubTest();
	pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP,
		"Michael Wilber");
	CPPUNIT_ASSERT(pbits == NULL);
	
	// Stream (open file, translate to BBitmapStream,
	// pass that BBitmapStream to GetBitmap)
	NextSubTest();
	BFile imgfile(
		"../src/tests/kits/translation/data/images/image.png",
		B_READ_ONLY);
	CPPUNIT_ASSERT(imgfile.InitCheck() == B_OK);
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster);
	BBitmapStream stream;
	CPPUNIT_ASSERT(proster->Translate(&imgfile, NULL, NULL,
		&stream, B_TRANSLATOR_BITMAP) == B_OK);
	pbits = BTranslationUtils::GetBitmap(&stream);
	CheckBitmap(pbits);
	delete pbits;
	pbits = NULL;
}

void
TranslationUtilsTest::GetPutStyledTextTest()
{
	// Insert text into view with styles OFF
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translationutilstest");
	BTextView *ptextview = NULL;
	ptextview = new BTextView(BRect(0, 0, 100, 100),
		"utilstest_textview", BRect(0, 0, 100, 100),
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);
	CPPUNIT_ASSERT(ptextview);
	ptextview->SetStylable(false);
	BFile stylfile(
		"../src/tests/kits/translation/data/text/sentence.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(stylfile.InitCheck() == B_OK);
	CPPUNIT_ASSERT(BTranslationUtils::GetStyledText(&stylfile, ptextview) == B_OK);
	CPPUNIT_ASSERT(ptextview->TextLength() == 77);
	CPPUNIT_ASSERT(strcmp(ptextview->Text(),
		"Ask not what open source can do for you, ask what you can do for open source.") == 0);
	
	// Insert text into view with styles ON
	NextSubTest();
	ptextview->SetText("");
	CPPUNIT_ASSERT(ptextview->TextLength() == 0);
	ptextview->SetStylable(true);
	CPPUNIT_ASSERT(BTranslationUtils::GetStyledText(&stylfile, ptextview) == B_OK);
	CPPUNIT_ASSERT(ptextview->TextLength() == 77);
	CPPUNIT_ASSERT(strcmp(ptextview->Text(),
		"Ask not what open source can do for you, ask what you can do for open source.") == 0);
		
	// Write current contents of the BTextView to a file, and make
	// sure that it matches the source file exactly
	NextSubTest();
	BFile tmpfile("/tmp/out_styled_text.stxt", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(tmpfile.InitCheck() == B_OK);
	CPPUNIT_ASSERT(BTranslationUtils::PutStyledText(ptextview, &tmpfile) == B_OK);
	off_t stylsize = 0, tmpsize = 0;
	CPPUNIT_ASSERT(stylfile.GetSize(&stylsize) == B_OK);
	CPPUNIT_ASSERT(tmpfile.GetSize(&tmpsize) == B_OK);
	CPPUNIT_ASSERT(stylsize == tmpsize);
	char *stylbuf = NULL, *tmpbuf = NULL;
	stylbuf = new char[stylsize];
	tmpbuf = new char[tmpsize];
	CPPUNIT_ASSERT(stylbuf && tmpbuf);
	CPPUNIT_ASSERT(stylfile.ReadAt(0, stylbuf, stylsize) == stylsize);
	CPPUNIT_ASSERT(tmpfile.ReadAt(0, tmpbuf, tmpsize) == tmpsize);
	CPPUNIT_ASSERT(memcmp(stylbuf, tmpbuf, stylsize) == 0);
	delete[] stylbuf;
	delete[] tmpbuf;
	stylbuf = NULL;
	tmpbuf = NULL;
	
	// Send NULL pointers to GetStyledText
	NextSubTest();
	CPPUNIT_ASSERT(BTranslationUtils::GetStyledText(NULL, ptextview) == B_BAD_VALUE);
	CPPUNIT_ASSERT(BTranslationUtils::GetStyledText(&stylfile, NULL) == B_BAD_VALUE);
	
	// Send NULL pointers to PutStyledText
	NextSubTest();
	CPPUNIT_ASSERT(BTranslationUtils::PutStyledText(NULL, &tmpfile) == B_BAD_VALUE);
	CPPUNIT_ASSERT(BTranslationUtils::PutStyledText(ptextview, NULL) == B_BAD_VALUE);
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
	
	// Try to get a translator that isn't there
	NextSubTest();
	pmsg = BTranslationUtils::GetDefaultSettings("Michael Wilber", 0);
	CPPUNIT_ASSERT(pmsg == NULL);
	
	// Try to get a version of a translator that we don't have
	NextSubTest();
	pmsg = BTranslationUtils::GetDefaultSettings("PPM Images", 1);
	CPPUNIT_ASSERT(pmsg == NULL);
}

void
TranslationUtilsTest::AddTranslationItemsTest()
{
	// Make menu of bitmap translators
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translationutilstest");
	BMenu *pmenu = new BMenu("utilstest_menu");
	CPPUNIT_ASSERT(pmenu);
	CPPUNIT_ASSERT(BTranslationUtils::AddTranslationItems(pmenu,
		B_TRANSLATOR_BITMAP) == B_OK);
	CPPUNIT_ASSERT(pmenu->CountItems() > 0);
	
	// Make menu of text translators
	NextSubTest();
	int32 nitem = 0;
	while (pmenu->CountItems())
		delete pmenu->RemoveItem(nitem);
	CPPUNIT_ASSERT(pmenu->CountItems() == 0);
	CPPUNIT_ASSERT(BTranslationUtils::AddTranslationItems(pmenu,
		B_TRANSLATOR_TEXT) == B_OK);
	CPPUNIT_ASSERT(pmenu->CountItems() > 0);
	
	// Make menu of 1 translator
	NextSubTest();
	while (pmenu->CountItems())
		delete pmenu->RemoveItem(nitem);
	CPPUNIT_ASSERT(pmenu->CountItems() == 0);
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/beos/system/add-ons/Translators/PPMTranslator"
		) == B_OK);
	CPPUNIT_ASSERT(BTranslationUtils::AddTranslationItems(pmenu,
		B_TRANSLATOR_BITMAP, NULL, NULL, NULL, proster) == B_OK);
	CPPUNIT_ASSERT(pmenu->CountItems() == 1);
	delete proster;
	proster = NULL;
	
	delete pmenu;
	pmenu = NULL;
	
	// Bad Input
	NextSubTest();
	CPPUNIT_ASSERT(BTranslationUtils::AddTranslationItems(NULL,
		B_TRANSLATOR_BITMAP) == B_BAD_VALUE);
}

