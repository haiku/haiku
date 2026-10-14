/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 */


#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Entry.h>
#include <File.h>
#include <Menu.h>
#include <MenuItem.h>
#include <OS.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include <stdio.h>
#include <string.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


static void
CheckBitmap(BBitmap* pbits)
{
	CPPUNIT_ASSERT(pbits);
	CPPUNIT_ASSERT(pbits->Bits());
	CPPUNIT_ASSERT(pbits->BitsLength() == 443904);
	CPPUNIT_ASSERT(pbits->BytesPerRow() == 1536);
	CPPUNIT_ASSERT(pbits->Bounds().IntegerWidth() == 383);
	CPPUNIT_ASSERT(pbits->Bounds().IntegerHeight() == 288);
}

class TranslationUtilsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TranslationUtilsTest);
	CPPUNIT_TEST(FilePath_GetBitmap_Succeeds);
	CPPUNIT_TEST(NonExistentFile_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(FilePath_GetBitmapFile_Succeeds);
	CPPUNIT_TEST(NonExistentFile_GetBitmapFile_ReturnsNull);
	CPPUNIT_TEST(EntryRef_GetBitmap_Succeeds);
	CPPUNIT_TEST(NullEntryRef_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(ResourceName_GetBitmap_Succeeds);
	CPPUNIT_TEST(InvalidResourceName_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(ResourceTypeAndId_GetBitmap_Succeeds);
	CPPUNIT_TEST(WrongResourceType_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(WrongResourceId_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(ResourceTypeAndName_GetBitmap_Succeeds);
	CPPUNIT_TEST(WrongResourceTypeName_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(WrongResourceNameValue_GetBitmap_ReturnsNull);
	CPPUNIT_TEST(Stream_GetBitmap_Succeeds);
	CPPUNIT_TEST(StylesOff_GetStyledText_Succeeds);
	CPPUNIT_TEST(StylesOn_GetStyledText_Succeeds);
	CPPUNIT_TEST(File_PutStyledText_MatchesOriginal);
	CPPUNIT_TEST(NullPointers_GetStyledText_ReturnsBadValue);
	CPPUNIT_TEST(NullPointers_PutStyledText_ReturnsBadValue);
	CPPUNIT_TEST(Roster_GetDefaultSettings_Succeeds);
	CPPUNIT_TEST(NoRoster_GetDefaultSettings_Succeeds);
	CPPUNIT_TEST(NonExistentTranslator_GetDefaultSettings_ReturnsNull);
	CPPUNIT_TEST(WrongVersion_GetDefaultSettings_ReturnsNull);
	CPPUNIT_TEST(BitmapTranslators_AddTranslationItems_Succeeds);
	CPPUNIT_TEST(TextTranslators_AddTranslationItems_Succeeds);
	CPPUNIT_TEST(SingleTranslator_AddTranslationItems_Succeeds);
	CPPUNIT_TEST(NullMenu_AddTranslationItems_ReturnsBadValue);
	CPPUNIT_TEST_SUITE_END();

public:
	void FilePath_GetBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap("resources/kits/translation/image.png");
		CheckBitmap(pbits);
		delete pbits;
	}

	void NonExistentFile_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap("/tmp/no-file-here.bmp");
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void FilePath_GetBitmapFile_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmapFile("resources/kits/translation/image.png");
		CheckBitmap(pbits);
		delete pbits;
	}

	void NonExistentFile_GetBitmapFile_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmapFile("/tmp/no-file-here.bmp");
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void EntryRef_GetBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		entry_ref ref;
		BEntry bent("resources/kits/translation/image.png");
		CPPUNIT_ASSERT_EQUAL(B_OK, bent.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, bent.GetRef(&ref));
		BBitmap* pbits = BTranslationUtils::GetBitmap(&ref);
		CheckBitmap(pbits);
		delete pbits;
	}

	void NullEntryRef_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		entry_ref* pref = NULL;
		BBitmap* pbits = BTranslationUtils::GetBitmap(pref);
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void ResourceName_GetBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap("res_image");
		CheckBitmap(pbits);
		delete pbits;
	}

	void InvalidResourceName_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap("Michael Wilber");
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void ResourceTypeAndId_GetBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP, 246);
		CheckBitmap(pbits);
		delete pbits;
	}

	void WrongResourceType_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_TEXT, 246);
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void WrongResourceId_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP, 166);
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void ResourceTypeAndName_GetBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP, "res_image");
		CheckBitmap(pbits);
		delete pbits;
	}

	void WrongResourceTypeName_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_TEXT, "res_image");
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void WrongResourceNameValue_GetBitmap_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BBitmap* pbits = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP, "Michael Wilber");
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void Stream_GetBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BFile imgfile("resources/kits/translation/image.png", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, imgfile.InitCheck());
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(proster);
		BBitmapStream stream;
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&imgfile, NULL, NULL, &stream, B_TRANSLATOR_BITMAP));
		BBitmap* pbits = BTranslationUtils::GetBitmap(&stream);
		CheckBitmap(pbits);
		delete pbits;
	}

	void StylesOff_GetStyledText_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BTextView* ptextview
			= new BTextView(BRect(0, 0, 100, 100), "utilstest_textview", BRect(0, 0, 100, 100),
							B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);
		CPPUNIT_ASSERT(ptextview);
		ptextview->SetStylable(false);
		BFile stylfile("resources/kits/translation/sentence.stxt", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, stylfile.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::GetStyledText(&stylfile, ptextview));
		CPPUNIT_ASSERT(ptextview->TextLength() == 77);
		CPPUNIT_ASSERT(
			strcmp(ptextview->Text(),
				   "Ask not what open source can do for you, ask what you can do for open source.")
			== 0);
		delete ptextview;
	}

	void StylesOn_GetStyledText_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BTextView* ptextview
			= new BTextView(BRect(0, 0, 100, 100), "utilstest_textview", BRect(0, 0, 100, 100),
							B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);
		CPPUNIT_ASSERT(ptextview);
		ptextview->SetStylable(true);
		BFile stylfile("resources/kits/translation/sentence.stxt", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, stylfile.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::GetStyledText(&stylfile, ptextview));
		CPPUNIT_ASSERT(ptextview->TextLength() == 77);
		CPPUNIT_ASSERT(
			strcmp(ptextview->Text(),
				   "Ask not what open source can do for you, ask what you can do for open source.")
			== 0);
		delete ptextview;
	}

	void File_PutStyledText_MatchesOriginal()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BTextView* ptextview
			= new BTextView(BRect(0, 0, 100, 100), "utilstest_textview", BRect(0, 0, 100, 100),
							B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);
		CPPUNIT_ASSERT(ptextview);
		ptextview->SetStylable(true);
		BFile stylfile("resources/kits/translation/sentence.stxt", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, stylfile.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::GetStyledText(&stylfile, ptextview));

		BFile tmpfile("/tmp/out_styled_text.stxt", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, tmpfile.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::PutStyledText(ptextview, &tmpfile));
		off_t stylsize = 0, tmpsize = 0;
		CPPUNIT_ASSERT_EQUAL(B_OK, stylfile.GetSize(&stylsize));
		CPPUNIT_ASSERT_EQUAL(B_OK, tmpfile.GetSize(&tmpsize));
		CPPUNIT_ASSERT(stylsize == tmpsize);
		char* stylbuf = new char[stylsize];
		char* tmpbuf = new char[tmpsize];
		CPPUNIT_ASSERT(stylbuf && tmpbuf);
		CPPUNIT_ASSERT(stylfile.ReadAt(0, stylbuf, stylsize) == stylsize);
		CPPUNIT_ASSERT(tmpfile.ReadAt(0, tmpbuf, tmpsize) == tmpsize);
		//	CPPUNIT_ASSERT(memcmp(stylbuf, tmpbuf, stylsize) == 0);
		delete[] stylbuf;
		delete[] tmpbuf;
		delete ptextview;
	}

	void NullPointers_GetStyledText_ReturnsBadValue()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BTextView* ptextview
			= new BTextView(BRect(0, 0, 100, 100), "utilstest_textview", BRect(0, 0, 100, 100),
							B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);
		BFile stylfile("resources/kits/translation/sentence.stxt", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BTranslationUtils::GetStyledText(NULL, ptextview));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BTranslationUtils::GetStyledText(&stylfile, NULL));
		delete ptextview;
	}

	void NullPointers_PutStyledText_ReturnsBadValue()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BTextView* ptextview
			= new BTextView(BRect(0, 0, 100, 100), "utilstest_textview", BRect(0, 0, 100, 100),
							B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);
		BFile tmpfile("/tmp/out_styled_text.stxt", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BTranslationUtils::PutStyledText(NULL, &tmpfile));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BTranslationUtils::PutStyledText(ptextview, NULL));
		delete ptextview;
	}

	void Roster_GetDefaultSettings_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		translator_id* pids = NULL;
		int32 ncount = 0;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(proster);
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetAllTranslators(&pids, &ncount));
		CPPUNIT_ASSERT(pids && ncount > 0);
		BMessage* pmsg = BTranslationUtils::GetDefaultSettings(pids[0], proster);
		CPPUNIT_ASSERT(pmsg);
		delete pmsg;
		delete[] pids;
	}

	void NoRoster_GetDefaultSettings_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		translator_id* pids = NULL;
		int32 ncount = 0;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(proster);
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetAllTranslators(&pids, &ncount));
		CPPUNIT_ASSERT(pids && ncount > 0);
		BMessage* pmsg = BTranslationUtils::GetDefaultSettings(pids[0]);
		CPPUNIT_ASSERT(pmsg);
		delete pmsg;
		delete[] pids;
	}

#if 0
	void TGATranslator_GetDefaultSettings_ReturnsCorrectSettings()
	{
		//	pmsg = BTranslationUtils::GetDefaultSettings("TGA Images", 100);
		//	CPPUNIT_ASSERT(pmsg);
		//	CPPUNIT_ASSERT(pmsg->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY,
		//		&bdummy) == B_OK);
		//	CPPUNIT_ASSERT(pmsg->FindBool(B_TRANSLATOR_EXT_DATA_ONLY,
		//		&bdummy) == B_OK);
		//	CPPUNIT_ASSERT(pmsg->FindBool("tga /rle", &bdummy) == B_OK);
		//	delete pmsg;
	}
#endif

	void NonExistentTranslator_GetDefaultSettings_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BMessage* pmsg = BTranslationUtils::GetDefaultSettings("Michael Wilber", 0);
		CPPUNIT_ASSERT(pmsg == NULL);
	}

	void WrongVersion_GetDefaultSettings_ReturnsNull()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BMessage* pmsg = BTranslationUtils::GetDefaultSettings("PPM Images", 1);
		CPPUNIT_ASSERT(pmsg == NULL);
	}

	void BitmapTranslators_AddTranslationItems_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BMenu* pmenu = new BMenu("utilstest_menu");
		CPPUNIT_ASSERT(pmenu);
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::AddTranslationItems(pmenu, B_TRANSLATOR_BITMAP));
		CPPUNIT_ASSERT(pmenu->CountItems() > 0);
		delete pmenu;
	}

	void TextTranslators_AddTranslationItems_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BMenu* pmenu = new BMenu("utilstest_menu");
		CPPUNIT_ASSERT(pmenu);
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::AddTranslationItems(pmenu, B_TRANSLATOR_TEXT));
		CPPUNIT_ASSERT(pmenu->CountItems() > 0);
		delete pmenu;
	}

	void SingleTranslator_AddTranslationItems_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		BMenu* pmenu = new BMenu("utilstest_menu");
		CPPUNIT_ASSERT(pmenu);
		BTranslatorRoster* proster = new BTranslatorRoster();
		CPPUNIT_ASSERT(proster);
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->AddTranslators("/boot/system/add-ons/Translators/PPMTranslator"));
		CPPUNIT_ASSERT_EQUAL(B_OK, BTranslationUtils::AddTranslationItems(pmenu, B_TRANSLATOR_BITMAP, NULL,
															  NULL, NULL, proster));
		// CPPUNIT_ASSERT(pmenu->CountItems() == 1);
		delete proster;
		delete pmenu;
	}

	void NullMenu_AddTranslationItems_ReturnsBadValue()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_translationutilstest");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BTranslationUtils::AddTranslationItems(NULL, B_TRANSLATOR_BITMAP));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TranslationUtilsTest, getTestSuiteName());
