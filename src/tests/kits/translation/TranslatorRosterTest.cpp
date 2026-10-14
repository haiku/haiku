/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Matzon <brian@matzon.dk>
 */

#include "Errors.h"
#include <Application.h>
#include <Archivable.h>
#include <File.h>
#include <Message.h>
#include <OS.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include <stdio.h>
#include <string.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

//
// Compares proster to the default BTranslatorRoster to
// ensure that they have the same translators and the
// same number of translators
//
static void
CompareWithDefault(BTranslatorRoster* proster)
{
	BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);

	translator_id *pInstids = NULL, *pDefids = NULL;
	int32 instcount = 0, defcount = 0, ndummy = 0;
	const char *strDefName = NULL, *strInstName = NULL, *strDummy = NULL;

	CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetAllTranslators(&pInstids, &instcount));
	CPPUNIT_ASSERT(pInstids);
	CPPUNIT_ASSERT(instcount > 0);

	CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&pDefids, &defcount));
	CPPUNIT_ASSERT(pDefids);
	CPPUNIT_ASSERT(defcount > 0);
	CPPUNIT_ASSERT(defcount == instcount);

	// make sure that every translator that is in the default
	// BTranslatorRoster is in proster, once and only once
	for (int32 i = 0; i < defcount; i++) {
		int32 matches;
		matches = 0;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetTranslatorInfo(pDefids[i], &strDefName, &strDummy, &ndummy));
		CPPUNIT_ASSERT(strDefName);
		for (int32 k = 0; k < instcount; k++) {
			CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetTranslatorInfo(pInstids[k], &strInstName, &strDummy, &ndummy));
			CPPUNIT_ASSERT(strInstName);

			if (strcmp(strDefName, strInstName) == 0)
				matches++;
		}
		CPPUNIT_ASSERT(matches == 1);
	}

	delete[] pInstids;
	pInstids = NULL;
	delete[] pDefids;
	pDefids = NULL;
}

static void
CheckTranslatorInfo(translator_info* pinfo, int32 nitems)
{
	for (int32 k = 0; k < nitems; k++) {
		CPPUNIT_ASSERT(pinfo[k].translator > 0);
		CPPUNIT_ASSERT(pinfo[k].type);
		CPPUNIT_ASSERT(pinfo[k].group);
		CPPUNIT_ASSERT(pinfo[k].quality >= 0 && pinfo[k].quality <= 1);
		CPPUNIT_ASSERT(pinfo[k].capability >= 0 && pinfo[k].capability <= 1);
		CPPUNIT_ASSERT(strlen(pinfo[k].MIME) >= 0);
		CPPUNIT_ASSERT(strlen(pinfo[k].name) > 0);
	}
}

class TranslatorRosterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TranslatorRosterTest);
	CPPUNIT_TEST(NoArgs_Constructor_Succeeds);
	CPPUNIT_TEST(BMessage_Constructor_Succeeds);
	CPPUNIT_TEST(Default_Succeeds);
	CPPUNIT_TEST(DeleteAndReacquire_Default_Succeeds);
	CPPUNIT_TEST(EmptyMessage_Instantiate_ReturnsNull);
	CPPUNIT_TEST(ValidMessage_Instantiate_ReturnsRoster);
	CPPUNIT_TEST(CorruptMessage_Instantiate_ReturnsNull);
	CPPUNIT_TEST(ValidEmptyMessage_Instantiate_ReturnsRoster);
	CPPUNIT_TEST(SpecificPaths_AddTranslators_Succeeds);
	CPPUNIT_TEST(DefaultPaths_AddTranslators_Succeeds);
	CPPUNIT_TEST(DefaultRoster_Archive_Succeeds);
	CPPUNIT_TEST(NullArguments_GetAllTranslators_ReturnsBadValue);
	CPPUNIT_TEST(NoTranslators_GetAllTranslators_Succeeds);
	CPPUNIT_TEST(DefaultRoster_GetAllTranslators_Succeeds);
	CPPUNIT_TEST(InvalidTranslator_GetConfigurationMessage_ReturnsNoTranslator);
	CPPUNIT_TEST(NullMessage_GetConfigurationMessage_ReturnsBadValue);
	CPPUNIT_TEST(ValidRoster_GetConfigurationMessage_Succeeds);
	CPPUNIT_TEST(AllTranslators_GetInputFormats_Succeeds);
	CPPUNIT_TEST(AllTranslators_GetOutputFormats_Succeeds);
	CPPUNIT_TEST(InvalidTranslator_GetTranslatorInfo_ReturnsNoTranslator);
	CPPUNIT_TEST(AllTranslators_GetTranslatorInfo_Succeeds);
	CPPUNIT_TEST(InvalidArguments_GetTranslators_ReturnsBadValue);
	CPPUNIT_TEST(ValidImage_GetTranslators_Succeeds);
	CPPUNIT_TEST(ValidMessage_GetTranslators_Succeeds);
	CPPUNIT_TEST(InvalidArguments_Identify_ReturnsBadValue);
	CPPUNIT_TEST(ValidImage_Identify_Succeeds);
	CPPUNIT_TEST(SpecificOutputFormat_Identify_Succeeds);
	CPPUNIT_TEST(InvalidArguments_MakeConfigurationView_ReturnsBadValue);
	CPPUNIT_TEST(InvalidTranslator_MakeConfigurationView_ReturnsNoTranslator);
	// CPPUNIT_TEST(ValidArgs_MakeConfigurationView_ReturnsValid);
	CPPUNIT_TEST(GenericOutputFormat_Translate_Succeeds);
	CPPUNIT_TEST(TGAOutputFormat_Translate_Succeeds);
	CPPUNIT_TEST_SUITE_END();

public:
	void NoArgs_Constructor_Succeeds()
	{
		BTranslatorRoster* proster = new BTranslatorRoster();
		CPPUNIT_ASSERT(proster != NULL);
		delete proster;
	}

	void BMessage_Constructor_Succeeds()
	{
		BMessage translator_message;
		translator_message.AddString("be:translator_path", "/boot/system/add-ons/Translators");
		BTranslatorRoster* proster = new BTranslatorRoster(&translator_message);
		CPPUNIT_ASSERT(proster != NULL);

		int32 nloaded = -42;
		translator_id* pids = NULL;
		CPPUNIT_ASSERT_EQUAL(B_NO_ERROR, proster->GetAllTranslators(&pids, &nloaded));
		CPPUNIT_ASSERT(nloaded > 0);
		CPPUNIT_ASSERT(pids != NULL);

		// make certain that each translator is valid
		for (int32 i = 0; i < nloaded; i++) {
			const char *kTranslatorName, *kTranslatorInfo;
			int32 nTranslatorVersion;

			CPPUNIT_ASSERT(pids[i] > 0);

			kTranslatorName = kTranslatorInfo = NULL;
			nTranslatorVersion = -246;

			proster->GetTranslatorInfo(pids[i], &kTranslatorName, &kTranslatorInfo,
									   &nTranslatorVersion);

			CPPUNIT_ASSERT(kTranslatorName);
			CPPUNIT_ASSERT(kTranslatorInfo);
			CPPUNIT_ASSERT(nTranslatorVersion > 0);
		}

		// TODO: make certain that each translator in kaTranslatorNames
		// is loaded exactly once

		delete[] pids;
		delete proster;
	}

	void Default_Succeeds()
	{
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(proster != NULL);
	}

	void DeleteAndReacquire_Default_Succeeds()
	{
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(proster != NULL);

		// delete the default BTranslatorRoster
		// (may not always be the appropriate thing to do,
		// but it should work without blowing up)
		delete proster;
		// If the default BTranslatorRoster was not
		// deleted properly, it would likely show up in
		// this test
		proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(proster);

		// Make sure the default BTranslatorRoster works
		// after a delete has been performed
		// TODO: actually check if all the right translators are loaded
		translator_id* pids = NULL;
		int32 ncount = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetAllTranslators(&pids, &ncount));
		CPPUNIT_ASSERT(pids);
		CPPUNIT_ASSERT(ncount > 0);
		delete[] pids;
	}

	void EmptyMessage_Instantiate_ReturnsNull()
	{
		BMessage bmsg;
		BTranslatorRoster* proster
			= dynamic_cast<BTranslatorRoster*>(BTranslatorRoster::Instantiate(&bmsg));
		CPPUNIT_ASSERT(proster == NULL);
	}

	// TODO: add a case with a BMessage containing multiple Translators to load

	void ValidMessage_Instantiate_ReturnsRoster()
	{
		BMessage bmsg;
		bmsg.AddString("class", "BTranslatorRoster");
		bmsg.AddString("be:translator_path", "/boot/system/add-ons/Translators/BMPTranslator");
		BTranslatorRoster* proster
			= dynamic_cast<BTranslatorRoster*>(BTranslatorRoster::Instantiate(&bmsg));
		CPPUNIT_ASSERT(proster);

		translator_id* pids = NULL;
		int32 ncount = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetAllTranslators(&pids, &ncount));
		CPPUNIT_ASSERT(pids);
		CPPUNIT_ASSERT(ncount == 1);

		// TODO: check name, issue: depends on locale
		delete proster;
		delete[] pids;
	}

	void ValidEmptyMessage_Instantiate_ReturnsRoster()
	{
				BMessage bmsg;
		bmsg.AddString("class", "BTranslatorRoster");
		BTranslatorRoster* proster
			= dynamic_cast<BTranslatorRoster*>(BTranslatorRoster::Instantiate(&bmsg));
		CPPUNIT_ASSERT(proster);

		translator_id* pids = NULL;
		int32 ncount = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->GetAllTranslators(&pids, &ncount));
		CPPUNIT_ASSERT(pids);
		CPPUNIT_ASSERT(ncount == 0);
		delete proster;
		delete[] pids;
	}

	void CorruptMessage_Instantiate_ReturnsNull()
	{
		BMessage bmsg;
		bmsg.AddString("be:translator_path", "/boot/system/add-ons/Translators/BMPTranslator");
		BTranslatorRoster* proster
			= dynamic_cast<BTranslatorRoster*>(BTranslatorRoster::Instantiate(&bmsg));
		CPPUNIT_ASSERT(proster == NULL);
	}

	void SpecificPaths_AddTranslators_Succeeds()
	{
		BTranslatorRoster* proster = new BTranslatorRoster();
		CPPUNIT_ASSERT(proster);
		CPPUNIT_ASSERT(proster->AddTranslators("/boot/home/config/add-ons/Translators/:"
											   "/system/add-ons/Translators/")
					   == B_OK);

		int32 instcount = 0;
		translator_id* translators = NULL;
		proster->GetAllTranslators(&translators, &instcount);
		CPPUNIT_ASSERT(translators);
		CPPUNIT_ASSERT(instcount > 0);
		// TODO: count the number of files in all of the directories specified
		// TODO: above and make certain that it matches instcount
		delete[] translators;
		delete proster;
	}

	void DefaultPaths_AddTranslators_Succeeds()
	{
		BTranslatorRoster* proster = new BTranslatorRoster();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->AddTranslators());

		// make sure that every translator in the Default BTranslatorRoster is in
		// proster, and make certain that it is in there ONLY ONCE
		CompareWithDefault(proster);
		delete proster;
	}

	void DefaultRoster_Archive_Succeeds()
	{
		// archive default, and count entries (must be more than 1!)
		BMessage translator_message;
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(pDefRoster);
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->Archive(&translator_message));

		// make sure instantiate makes an "exact" copy of the default translator
		BTranslatorRoster* proster
			= dynamic_cast<BTranslatorRoster*>(BTranslatorRoster::Instantiate(&translator_message));
		CPPUNIT_ASSERT(proster);

		// make sure that every translator in the pDefRoster is in
		// proster, and make certain that it is in there ONLY ONCE
		CompareWithDefault(proster);
		delete proster;
	}

	void NullArguments_GetAllTranslators_ReturnsBadValue()
	{
		int32 nloaded = 42;
		translator_id* pids = NULL;
		BTranslatorRoster roster;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, roster.GetAllTranslators(NULL, &nloaded));
		CPPUNIT_ASSERT(nloaded == 42);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, roster.GetAllTranslators(&pids, NULL));
		CPPUNIT_ASSERT(pids == NULL);
	}

	void NoTranslators_GetAllTranslators_Succeeds()
	{
		BTranslatorRoster roster;
		translator_id* pids = NULL;
		int32 nloaded = -1;
		CPPUNIT_ASSERT_EQUAL(B_NO_ERROR, roster.GetAllTranslators(&pids, &nloaded));
		CPPUNIT_ASSERT(nloaded == 0);
		delete[] pids;
	}

	void DefaultRoster_GetAllTranslators_Succeeds()
	{
		int32 nloaded = 42;
		translator_id* pids = NULL;
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_NO_ERROR, pDefRoster->GetAllTranslators(&pids, &nloaded));
		CPPUNIT_ASSERT(nloaded > 0);
		CPPUNIT_ASSERT(pids != NULL);

		for (int32 i = 0; i < nloaded; i++) {
			// TODO: make sure correct translators were loaded
			const char *kTranslatorName = NULL, *kTranslatorInfo = NULL;
			int32 nTranslatorVersion = -1;

			CPPUNIT_ASSERT(pids[i] > 0);
			CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetTranslatorInfo(pids[i], &kTranslatorName,
				&kTranslatorInfo, &nTranslatorVersion));
			CPPUNIT_ASSERT(kTranslatorName);
			CPPUNIT_ASSERT(kTranslatorInfo);
			CPPUNIT_ASSERT(nTranslatorVersion > 0);
		}

		delete[] pids;
	}

	void InvalidTranslator_GetConfigurationMessage_ReturnsNoTranslator()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		BMessage translator_message;
		CPPUNIT_ASSERT_EQUAL(B_NO_TRANSLATOR, pDefRoster->GetConfigurationMessage(-1, &translator_message));
		CPPUNIT_ASSERT(translator_message.IsEmpty());
	}

	void NullMessage_GetConfigurationMessage_ReturnsBadValue()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		int32 num_translators = -1;
		translator_id* translators = NULL;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE,
			pDefRoster->GetConfigurationMessage(translators[0], NULL));
		delete[] translators;
	}

	void ValidRoster_GetConfigurationMessage_Succeeds()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		int32 num_translators = -1;
		translator_id* translators = NULL;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);

		BMessage translator_message;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetConfigurationMessage(translators[0], &translator_message));
		delete[] translators;
	}

	void AllTranslators_GetInputFormats_Succeeds()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_id* translators = NULL;
		int32 num_translators = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);

		for (int32 i = 0; i < num_translators; i++) {
			const translation_format* fmts = NULL;
			int32 num_fmts = -1;
			CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetInputFormats(translators[i], &fmts, &num_fmts));
			CPPUNIT_ASSERT(num_fmts >= 0);
			CPPUNIT_ASSERT(num_fmts == 0 || fmts);
			for (int32 k = 0; k < num_fmts; k++) {
				CPPUNIT_ASSERT(fmts[k].type);
				CPPUNIT_ASSERT(fmts[k].group);
				CPPUNIT_ASSERT(fmts[k].name[0] != '\0');
			}
		}
		delete[] translators;
	}

	void AllTranslators_GetOutputFormats_Succeeds()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_id* translators = NULL;
		int32 num_translators = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);

		for (int32 i = 0; i < num_translators; i++) {
			const translation_format* fmts = NULL;
			int32 num_fmts = -1;
			CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetOutputFormats(translators[i], &fmts, &num_fmts));
			CPPUNIT_ASSERT(num_fmts >= 0);
			CPPUNIT_ASSERT(num_fmts == 0 || fmts);
			for (int32 k = 0; k < num_fmts; k++) {
				CPPUNIT_ASSERT(fmts[k].type);
				CPPUNIT_ASSERT(fmts[k].group);
				CPPUNIT_ASSERT(fmts[k].name[0] != '\0');
			}
		}
		delete[] translators;
	}

	void InvalidTranslator_GetTranslatorInfo_ReturnsNoTranslator()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		const char* outName = NULL;
		const char* outInfo = NULL;
		int32 outVersion = -1;
		CPPUNIT_ASSERT_EQUAL(B_NO_TRANSLATOR, pDefRoster->GetTranslatorInfo(-1, &outName, &outInfo, &outVersion));
	}

	void AllTranslators_GetTranslatorInfo_Succeeds()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_id* translators = NULL;
		int32 num_translators = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);

		for (int32 i = 0; i < num_translators; i++) {
			const char* outName = NULL;
			const char* outInfo = NULL;
			int32 outVersion = -1;
			CPPUNIT_ASSERT(
				pDefRoster->GetTranslatorInfo(translators[i], &outName, &outInfo, &outVersion)
				== B_OK);
			CPPUNIT_ASSERT(outName);
		}
		delete[] translators;
	}

#if 0
	void InvalidArguments_GetTranslatorInfo_ReturnsBadValue()
	{
//		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
//			NULL, &outInfo, &outVersion) == B_BAD_VALUE);
//		CPPUNIT_ASSERT(outInfo == NULL && outVersion == -1);
		
//		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
//			&outName, NULL, &outVersion) == B_BAD_VALUE);
//		CPPUNIT_ASSERT(outName == NULL && outVersion == -1);
//		
//		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
//			&outName, &outInfo, NULL) == B_BAD_VALUE);
//		CPPUNIT_ASSERT(outName == NULL && outInfo == NULL);
		
//		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
//			NULL, NULL, NULL) == B_BAD_VALUE);
	}
#endif

	void InvalidArguments_GetTranslators_ReturnsBadValue()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		BFile garbled("resources/kits/translation/garbled_data", B_READ_ONLY);
		int32 outCount = -1;
		translator_info* pinfo = NULL;

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pDefRoster->GetTranslators(&garbled, NULL, NULL, &outCount));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pDefRoster->GetTranslators(&garbled, NULL, &pinfo, NULL));
		// CPPUNIT_ASSERT_EQUAL(B_NO_TRANSLATOR, pDefRoster->GetTranslators(&garbled, NULL, &pinfo, &outCount));
	}

	void ValidImage_GetTranslators_Succeeds()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
		BFile image("resources/kits/translation/image.png", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, image.InitCheck());

		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_info* pinfo = NULL;
		int32 outCount = -1;

		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetTranslators(&image, NULL, &pinfo, &outCount));
		CPPUNIT_ASSERT(outCount > 0);
		CheckTranslatorInfo(pinfo, outCount);
		delete[] pinfo;
	}


	void ValidMessage_GetTranslators_Succeeds()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
		BFile image("resources/kits/translation/image.png", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, image.InitCheck());
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_info* pinfo = NULL;
		int32 outCount = -1;

		BMessage bmsg;
		CPPUNIT_ASSERT_EQUAL(B_OK, bmsg.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetTranslators(&image, NULL, &pinfo, &outCount));
		CPPUNIT_ASSERT(outCount > 0);
		CheckTranslatorInfo(pinfo, outCount);
		delete[] pinfo;
	}

	void InvalidArguments_Identify_ReturnsBadValue()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		BFile garbled("resources/kits/translation/garbled_data", B_READ_ONLY);
		translator_info info;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pDefRoster->Identify(&garbled, NULL, NULL));
		// CPPUNIT_ASSERT_EQUAL(B_NO_TRANSLATOR, pDefRoster->Identify(&garbled, NULL, &info));
	}

	void ValidImage_Identify_Succeeds()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
		BFile image("resources/kits/translation/image.png", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, image.InitCheck());

		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_info info;
		memset(&info, 0, sizeof(translator_info));
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->Identify(&image, NULL, &info));
		CheckTranslatorInfo(&info, 1);
	}

	void SpecificOutputFormat_Identify_Succeeds()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
		BFile image("resources/kits/translation/image.png", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, image.InitCheck());

		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_info info;
		memset(&info, 0, sizeof(translator_info));
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->Identify(&image, NULL, &info, 0, NULL, B_TRANSLATOR_BITMAP));
		CheckTranslatorInfo(&info, 1);
			
		// TODO: Add a test where I actually use pinfo and a BMessage 
		// TODO: and check their contents
	}

	void InvalidArguments_MakeConfigurationView_ReturnsBadValue()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_id* translators = NULL;
		int32 num_translators = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);

		BView* view = NULL;
		BRect extent;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pDefRoster->MakeConfigurationView(translators[0], NULL, &view, NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pDefRoster->MakeConfigurationView(translators[0], NULL, NULL, &extent));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pDefRoster->MakeConfigurationView(translators[0], NULL, NULL, NULL));
		delete[] translators;
	}

	void InvalidTranslator_MakeConfigurationView_ReturnsNoTranslator()
	{
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		BView* view = NULL;
		BRect extent;
		CPPUNIT_ASSERT_EQUAL(B_NO_TRANSLATOR, pDefRoster->MakeConfigurationView(-1, NULL, &view, &extent));
	}

	void ValidArgs_MakeConfigurationView_ReturnsValid()
	{	
		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		translator_id* translators = NULL;
		int32 num_translators = -1;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->GetAllTranslators(&translators, &num_translators));
		CPPUNIT_ASSERT(num_translators > 0);
		BView* view = NULL;
		BRect extent;
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->MakeConfigurationView(translators[0], NULL,
			&view, &extent));
		CPPUNIT_ASSERT(extent.IsValid() == true);
		
		delete[] translators;
		// TODO: Add a test that uses a valid BMessage with actual settings in it
	}

	void GenericOutputFormat_Translate_Succeeds()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
		BFile input("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, input.InitCheck());

		BFile temp("/tmp/TranslatorRosterTest.temp", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, temp.InitCheck());

		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT(pDefRoster != NULL);

		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->Translate(&input, NULL, NULL, &temp, B_TRANSLATOR_BITMAP));
	}

	void TGAOutputFormat_Translate_Succeeds()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
		BFile input("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, input.InitCheck());

		BFile temp("/tmp/TranslatorRosterTest.temp", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, temp.InitCheck());

		BTranslatorRoster* pDefRoster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->Translate(&input, NULL, NULL, &temp, B_TRANSLATOR_BITMAP));

		BFile output("resources/kits/translation/image.out.tga",
					 B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, output.InitCheck());

		CPPUNIT_ASSERT_EQUAL(B_OK, pDefRoster->Translate(&temp, NULL, NULL, &output, B_TGA_FORMAT));
	}

	// TODO: add Translate using ioExtension BMessage and whatever
	// TODO: the other NULL parameter is
};




CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TranslatorRosterTest, getTestSuiteName());
