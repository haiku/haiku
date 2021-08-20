/*****************************************************************************/
// Haiku Translation Kit Test
// Author: Brian Matzon <brian@matzon.dk>
// Version: 0.1.0
//
// This is the Test application for BTranslatorRoster
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
#include "TranslatorRosterTest.h"

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Archivable.h>
#include <File.h>
#include <Message.h>
#include <OS.h>
#include <TranslatorFormats.h>

/* cppunit framework */
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

/**
 * TestCase constructor - no work
 */
TranslatorRosterTest::TranslatorRosterTest(std::string name)
	: BTestCase(name)
{
}

/**
 * Default destructor - no work
 */
TranslatorRosterTest::~TranslatorRosterTest()
{
}

CppUnit::Test *
TranslatorRosterTest::Suite()
{
	/* create our suite */
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("TranslatorRoster");
		
	/* add suckers */
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Initialize Test",
		&TranslatorRosterTest::InitializeTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Constructor Test",
		&TranslatorRosterTest::ConstructorTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Default Test",
		&TranslatorRosterTest::DefaultTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Instantiate Test",
		&TranslatorRosterTest::InstantiateTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Version Test",
		&TranslatorRosterTest::VersionTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::AddTranslators Test",
		&TranslatorRosterTest::AddTranslatorsTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Archive Test",
		&TranslatorRosterTest::ArchiveTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::GetAllTranslators Test",
		&TranslatorRosterTest::GetAllTranslatorsTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::GetConfigurationMessage Test",
		&TranslatorRosterTest::GetConfigurationMessageTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::GetInputFormats Test",
		&TranslatorRosterTest::GetInputFormatsTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::GetOutputFormats Test",
		&TranslatorRosterTest::GetOutputFormatsTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::GetTranslatorInfo Test",
		&TranslatorRosterTest::GetTranslatorInfoTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::GetTranslators Test",
		&TranslatorRosterTest::GetTranslatorsTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Identify Test",
		&TranslatorRosterTest::IdentifyTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::MakeConfigurationView Test",
		&TranslatorRosterTest::MakeConfigurationViewTest));
		
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>(
		"TranslatorRosterTest::Translate Test",
		&TranslatorRosterTest::TranslateTest));			

	return suite;
}

/**
 * Tries to aquire the default roster
 */
void 
TranslatorRosterTest::InitializeTest()
{
	//aquire default roster
	NextSubTest();
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster);
}

/**
 * Construct proster using different kinds of constructors
 */
void 
TranslatorRosterTest::ConstructorTest()
{
	//shared instance of TranslatorRoster
	BTranslatorRoster *proster;

	// Create TranslatorRoster using noargs constructor
	// (GetAllTranslatorsTest also tests this constructor)
	NextSubTest();
	proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster != NULL);
	delete proster;
	proster = NULL;
	
	//Create TranslatorRoster using BMessage constructor
	NextSubTest();
	BMessage translator_message;
	translator_message.AddString("be:translator_path",
		"/boot/home/config/add-ons/Translators");
	proster = new BTranslatorRoster(&translator_message);
	CPPUNIT_ASSERT(proster != NULL);
	
	// Make sure the correct number of translators were loaded
	NextSubTest();
	int32 nloaded = -42;
	translator_id *pids = NULL;
	CPPUNIT_ASSERT(proster->GetAllTranslators(&pids, &nloaded) == B_NO_ERROR);
	CPPUNIT_ASSERT(nloaded > 0);
	CPPUNIT_ASSERT(pids != NULL);
	
	// Make sure the correct translators were loaded
	NextSubTest();
	const char *kaTranslatorNames[] = {
		"TGA Images",
		"MNGTranslator",
		"BMP Images",
		"TestBmpTranslator",
		"BBitmap Images",
		"GIMPPatternHandler"
	};
	int32 naCounts[sizeof(kaTranslatorNames) /
		sizeof(const char *)] = { 0 };
	CPPUNIT_ASSERT(nloaded == sizeof(kaTranslatorNames) /
		sizeof(const char *));
	
	for (int32 i = 0; i < nloaded; i++) {
		const char *kTranslatorName, *kTranslatorInfo;
		int32 nTranslatorVersion;
		
		CPPUNIT_ASSERT(pids[i] > 0);
		
		kTranslatorName = kTranslatorInfo = NULL;
		nTranslatorVersion = -246;
		
		proster->GetTranslatorInfo(pids[i], &kTranslatorName,
			&kTranslatorInfo, &nTranslatorVersion);
			
		CPPUNIT_ASSERT(kTranslatorName);
		CPPUNIT_ASSERT(kTranslatorInfo);
		CPPUNIT_ASSERT(nTranslatorVersion > 0);
		
		// make sure that the translator matches
		// one from the list
		int32 npresent = 0;
		for (int32 k = 0; k < nloaded; k++) {
			if (!strcmp(kaTranslatorNames[k], kTranslatorName)) {
				npresent++;
				naCounts[k]++;
			}
		}
		CPPUNIT_ASSERT(npresent == 1);
	}
	
	// make certain that each translator in kaTranslatorNames
	// is loaded exactly once
	for (int32 i = 0; i < nloaded; i++)
		CPPUNIT_ASSERT(naCounts[i] == 1);
	
	delete proster;
	proster = NULL;
}

/**
 * Tests:
 * BTranslatorRoster *Default()
 */
void
TranslatorRosterTest::DefaultTest()
{
	NextSubTest();
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster != NULL);
	// TODO: actually check if all the right translators are loaded
	
	// delete the default BTranslatorRoster
	// (may not always be the appropriate thing to do,
	// but it should work without blowing up)
	NextSubTest();
	delete proster;
	proster = NULL;
	
	// If the default BTranslatorRoster was not
	// deleted properly, it would likely show up in
	// this test or the next
	NextSubTest();
	proster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(proster);
	
	// Make sure the default BTranslatorRoster works
	// after a delete has been performed
	// TODO: actually check if all the right translators are loaded
	NextSubTest();
	translator_id *pids = NULL;
	int32 ncount = -1;
	CPPUNIT_ASSERT(proster->GetAllTranslators(&pids, &ncount) == B_OK);
	CPPUNIT_ASSERT(pids);
	CPPUNIT_ASSERT(ncount > 0);
	delete[] pids;
	pids = NULL;
	
	// Delete again to be sure that it still won't blow up
	NextSubTest();
	delete proster;
	proster = NULL;
}

/**
 * Tests:
 * BTranslatorRoster *Instantiate(BMessage *from)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
// ArchiveTest() also tests BTranslatorRoster::Instantiate()
void
TranslatorRosterTest::InstantiateTest()
{
	//shared instance of TranslatorRoster
	BTranslatorRoster* proster = NULL;
	BMessage bmsg;
		
	//create BTranslatorRoster using empty message (must return NULL)
	NextSubTest();
	proster = dynamic_cast<BTranslatorRoster *>
		(BTranslatorRoster::Instantiate(&bmsg));
	CPPUNIT_ASSERT(proster == NULL);
	delete proster;
	proster = NULL;
	
	// BMessage containing a single Translator to load
	NextSubTest();
	status_t result;
	result = bmsg.AddString("class", "BTranslatorRoster");
	CPPUNIT_ASSERT(result == B_OK);
	result = bmsg.AddString("be:translator_path",
		"/boot/home/config/add-ons/Translators/BMPTranslator");
	CPPUNIT_ASSERT(result == B_OK);
	proster = dynamic_cast<BTranslatorRoster *>
		(BTranslatorRoster::Instantiate(&bmsg));
	CPPUNIT_ASSERT(proster);
	
	translator_id *pids = NULL;
	int32 ncount = -1;
	result = proster->GetAllTranslators(&pids, &ncount);
	CPPUNIT_ASSERT(result == B_OK);
	CPPUNIT_ASSERT(pids);
	CPPUNIT_ASSERT(ncount == 1);
	const char *strName = NULL, *strInfo = NULL;
	int32 nversion = -1;
	result = proster->GetTranslatorInfo(pids[0], &strName, &strInfo,
		&nversion);
	CPPUNIT_ASSERT(result == B_OK);
	CPPUNIT_ASSERT(strName);
	CPPUNIT_ASSERT(strInfo);
	CPPUNIT_ASSERT(nversion > 0);
	CPPUNIT_ASSERT(strcmp("BMP Images", strName) == 0);
	delete proster;
	proster = NULL;
	
	// BMessage is valid, but does not contain any translators
	NextSubTest();
	result = bmsg.MakeEmpty();
	CPPUNIT_ASSERT(result == B_OK);
	result = bmsg.AddString("class", "BTranslatorRoster");
	CPPUNIT_ASSERT(result == B_OK);
	proster = dynamic_cast<BTranslatorRoster *>
		(BTranslatorRoster::Instantiate(&bmsg));
	CPPUNIT_ASSERT(proster);
	delete proster;
	proster = NULL;	
			
	// TODO: add a case with a BMessage containing multiple Translators to load
	
	// slightly corrupt BMessage, Instantiate 
	// should fail because class information is missing
	NextSubTest();
	result = bmsg.MakeEmpty();
	CPPUNIT_ASSERT(result == B_OK);
	result = bmsg.AddString("be:translator_path",
		"/boot/home/config/add-ons/Translators/BMPTranslator");
	CPPUNIT_ASSERT(result == B_OK);
	proster = dynamic_cast<BTranslatorRoster *>
		(BTranslatorRoster::Instantiate(&bmsg));
	CPPUNIT_ASSERT(proster == NULL);
	delete proster;
	proster = NULL;
}

/**
 * Tests:
 * const char *Version(int32 *outCurVersion, int32 *outMinVersion,
 * int32 *inAppVersion = B_TRANSLATION_CURRENT_VERSION)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::VersionTest()
{
#if 0
	NextSubTest();
	int32 outCurVersion = 0;
	int32 outMinVersion = 0;
	const char* info = NULL;
	
	info = BTranslatorRoster::Version(&outCurVersion, &outMinVersion);
	CPPUNIT_ASSERT(info != NULL);
	CPPUNIT_ASSERT(outCurVersion > 0);
	CPPUNIT_ASSERT(outMinVersion > 0);
#endif
}

//
// Compares proster to the default BTranslatorRoster to
// ensure that they have the same translators and the
// same number of translators
//
void
CompareWithDefault(BTranslatorRoster *proster)
{
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);
	
	translator_id *pInstids = NULL, *pDefids = NULL;
	int32 instcount = 0, defcount = 0, ndummy = 0;
	const char *strDefName = NULL, *strInstName = NULL, *strDummy = NULL;
	
	CPPUNIT_ASSERT(proster->GetAllTranslators(&pInstids, &instcount) == B_OK);
	CPPUNIT_ASSERT(pInstids);
	CPPUNIT_ASSERT(instcount > 0);
	
	CPPUNIT_ASSERT(pDefRoster->GetAllTranslators(&pDefids, &defcount) == B_OK);
	CPPUNIT_ASSERT(pDefids);
	CPPUNIT_ASSERT(defcount > 0);
	CPPUNIT_ASSERT(defcount == instcount);
	
	// make sure that every translator that is in the default
	// BTranslatorRoster is in proster, once and only once
	for (int32 i = 0; i < defcount; i++) {
		int32 matches;
		matches = 0;
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(pDefids[i], &strDefName,
			&strDummy, &ndummy) == B_OK);
		CPPUNIT_ASSERT(strDefName);
		for (int32 k = 0; k < instcount; k++) {
			CPPUNIT_ASSERT(proster->GetTranslatorInfo(pInstids[k],
				&strInstName, &strDummy, &ndummy) == B_OK);
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

/**
 * Tests:
 * virtual status_t AddTranslators(const char *load_path = NULL) 
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::AddTranslatorsTest()
{
	//create basic translatorroster
	NextSubTest();
	BTranslatorRoster* proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/:"
		"/system/add-ons/Translators/") == B_OK);

	NextSubTest();
	int32 instcount = 0;
	translator_id* translators = NULL;
	proster->GetAllTranslators(&translators, &instcount);
	CPPUNIT_ASSERT(translators);
	CPPUNIT_ASSERT(instcount > 0);
	// TODO: count the number of files in all of the directories specified
	// TODO: above and make certain that it matches instcount
	delete[] translators;
	translators = NULL;
	
	//delete and create new, this time don't specify path
	NextSubTest();
	delete proster;
	proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster->AddTranslators() == B_OK);
	
	// make sure that every translator in the Default BTranslatorRoster is in
	// proster, and make certain that it is in there ONLY ONCE
	NextSubTest();
	CompareWithDefault(proster);
	delete proster;
	proster = NULL;
}

/**
 * Tests:
 * virtual status_t Archive(BMessage *into, bool deep = true) const
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::ArchiveTest()
{
	NextSubTest();
	//archive default, and count entries (must be more than 1!)
	BMessage translator_message;
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);
	CPPUNIT_ASSERT(pDefRoster->Archive(&translator_message) == B_OK);

	// make sure instantiate makes an "exact" copy of the default translator
	NextSubTest();
	BTranslatorRoster *proster = NULL;
	proster = dynamic_cast<BTranslatorRoster *>
		(BTranslatorRoster::Instantiate(&translator_message));
	CPPUNIT_ASSERT(proster);
	
	// make sure that every translator in the pDefRoster is in
	// proster, and make certain that it is in there ONLY ONCE
	NextSubTest();
	CompareWithDefault(proster);
	delete proster;
	proster = NULL;
}

/**
 * Tests:
 * virtual status_t GetAllTranslators(translator_id **outList, int32 *outCount)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::GetAllTranslatorsTest()
{	
	// bad parameters
	NextSubTest();
	int32 nloaded = 42;
	translator_id* pids = NULL;
	BTranslatorRoster roster;
	CPPUNIT_ASSERT(roster.GetAllTranslators(NULL, &nloaded) == B_BAD_VALUE);
	CPPUNIT_ASSERT(nloaded == 42);
	CPPUNIT_ASSERT(roster.GetAllTranslators(&pids, NULL) == B_BAD_VALUE);
	CPPUNIT_ASSERT(pids == NULL);
	CPPUNIT_ASSERT(roster.GetAllTranslators(NULL, NULL) == B_BAD_VALUE);
	
	// no translators
	NextSubTest();
	CPPUNIT_ASSERT(
		roster.GetAllTranslators(&pids, &nloaded) == B_NO_ERROR);
	CPPUNIT_ASSERT(nloaded == 0);
	delete[] pids;
	pids = NULL;
	
	// default translators
	NextSubTest();
	nloaded = 42;
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();	
	CPPUNIT_ASSERT(
		pDefRoster->GetAllTranslators(&pids, &nloaded) == B_NO_ERROR);
	CPPUNIT_ASSERT(nloaded > 0);
	CPPUNIT_ASSERT(pids != NULL);
	
	// Make sure the correct translators were loaded
	NextSubTest();
	const char *kaTranslatorNames[] = {
		"Gobe MS-WORD Translator",
		"Gobe Text Translator",
		"TIFF Images",
		"Gobe SYLK Translator",
		"StyledEdit Files",
		"Gobe RTF Translator",
		"PPM Images",
		"JPEG Images",
		"Gobe HTML Translator",
		"Gobe Excel Translator",
		"TGA Images",
		"MNGTranslator",
		"GIMPPatternHandler",
		"TestBmpTranslator",
		"BMP Images",
		"BBitmap Images"
	};
	CPPUNIT_ASSERT(nloaded == sizeof(kaTranslatorNames) /
		sizeof(const char *));
	
	for (int32 i = 0; i < nloaded; i++) {
		const char *kTranslatorName, *kTranslatorInfo;
		int32 nTranslatorVersion;
		
		CPPUNIT_ASSERT(pids[i] > 0);
		
		kTranslatorName = kTranslatorInfo = NULL;
		nTranslatorVersion = -246;
		
		pDefRoster->GetTranslatorInfo(pids[i], &kTranslatorName,
			&kTranslatorInfo, &nTranslatorVersion);
			
		CPPUNIT_ASSERT(kTranslatorName);
		CPPUNIT_ASSERT(kTranslatorInfo);
		CPPUNIT_ASSERT(nTranslatorVersion > 0);
		
		// make sure each translator is loaded exactly once
		// no more, no less
		int32 npassed = 0;
		for (int32 k = 0; k < nloaded; k++) {
			if (!strcmp(kaTranslatorNames[k], kTranslatorName))
				npassed++;
		}
		CPPUNIT_ASSERT(npassed == 1);
	}

	delete[] pids;
	pids = NULL;
}

/**
 * Tests:
 * virtual status_t GetConfigurationMessage(translator_id forTranslator,
 * BMessage *ioExtension)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::GetConfigurationMessageTest()
{
	NextSubTest();
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);

	NextSubTest();
	//get id for a translator (just use the first one)
	int32 num_translators = -1;
	translator_id* translators = NULL;
	CPPUNIT_ASSERT(pDefRoster->GetAllTranslators(&translators,
		&num_translators) == B_OK);
	CPPUNIT_ASSERT(translators);
	CPPUNIT_ASSERT(num_translators > 0);
	
	NextSubTest();
	translator_id translatorid = translators[0];
	delete[] translators;
	translators = NULL;
	//get conf for invalid translator
	BMessage translator_message;
	CPPUNIT_ASSERT(pDefRoster->GetConfigurationMessage(-1,
		&translator_message) == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(translator_message.IsEmpty());
	
	//get conf for invalid ioExtension (BMessage)
	NextSubTest();
	CPPUNIT_ASSERT(
		pDefRoster->GetConfigurationMessage(translatorid, NULL) == B_BAD_VALUE);
	
	//get config for actual translator
	NextSubTest();
	CPPUNIT_ASSERT(translator_message.MakeEmpty() == B_OK);
	CPPUNIT_ASSERT(pDefRoster->GetConfigurationMessage(translatorid,
		&translator_message) == B_OK);
}

// Code used by both GetInputFormatsTest and GetOutputFormatsTest
void
GetInputOutputFormatsTest(TranslatorRosterTest *prt, bool binput)
{
	prt->NextSubTest();
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);
	
	prt->NextSubTest();
	translator_id* translators = NULL;
	int32 num_translators = -1;
	CPPUNIT_ASSERT(pDefRoster->GetAllTranslators(&translators,
		&num_translators) == B_OK);
	CPPUNIT_ASSERT(translators);
	CPPUNIT_ASSERT(num_translators > 0);

	// Run get Input/output format test on each translator
	for (int32 i = 0; i < num_translators; i++) {
		const translation_format *fmts = NULL;
		int32 num_fmts = -1;
		status_t result;
		prt->NextSubTest();
		if (binput)
			result = pDefRoster->GetInputFormats(translators[i], &fmts,
				&num_fmts);
		else
			result = pDefRoster->GetOutputFormats(translators[i], &fmts,
				&num_fmts);
		CPPUNIT_ASSERT(result == B_OK);
		
		// test input/output formats
		CPPUNIT_ASSERT(num_fmts >= 0);
		CPPUNIT_ASSERT(num_fmts == 0 || fmts);
		for (int32 k = 0; k < num_fmts; k++) {
			CPPUNIT_ASSERT(fmts[k].type);
			CPPUNIT_ASSERT(fmts[k].group);
			CPPUNIT_ASSERT(fmts[k].quality >= 0 && fmts[k].quality <= 1);
			CPPUNIT_ASSERT(fmts[k].capability >= 0 && fmts[k].capability <= 1);
			CPPUNIT_ASSERT(strlen(fmts[k].MIME) >= 0);
			CPPUNIT_ASSERT(strlen(fmts[k].name) > 0);
		}
	}
	delete[] translators;
	translators = NULL;
}

/**
 * Tests:
 * virtual status_t GetInputFormats(translator_id forTranslator, 
 * const translation_format **outFormats, int32 *outNumFormats) 
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::GetInputFormatsTest()
{
	GetInputOutputFormatsTest(this, true);
}

/**
 * Tests:
 * virtual status_t GetOutputFormats(translator_id forTranslator,
 * const translation_format **outFormats, int32 *outNumFormats)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::GetOutputFormatsTest()
{
	GetInputOutputFormatsTest(this, false);
}

/**
 * Tests:
 * virtual status_t GetTranslatorInfo(translator_id forTranslator,
 * const char **outName, const char **outInfo, int32 *outVersion)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::GetTranslatorInfoTest()
{
	NextSubTest();
	translator_id* translators = NULL;
	int32 num_translators = -1;
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);
	
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->GetAllTranslators(&translators,
		&num_translators) == B_OK);
	CPPUNIT_ASSERT(translators);
	CPPUNIT_ASSERT(num_translators > 0);
	for (int32 i = 0; i < num_translators; i++) {
		const char *outName = NULL;
		const char *outInfo = NULL;
		int32 outVersion = -1;
		
		NextSubTest();
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(-1, &outName, &outInfo,
			&outVersion) == B_NO_TRANSLATOR);
			
		// B_BAD_VALUE cases
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
			NULL, &outInfo, &outVersion) == B_BAD_VALUE);
		CPPUNIT_ASSERT(outInfo == NULL && outVersion == -1);
		
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
			&outName, NULL, &outVersion) == B_BAD_VALUE);
		CPPUNIT_ASSERT(outName == NULL && outVersion == -1);
		
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
			&outName, &outInfo, NULL) == B_BAD_VALUE);
		CPPUNIT_ASSERT(outName == NULL && outInfo == NULL);
		
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
			NULL, NULL, NULL) == B_BAD_VALUE);		
		
		// Good values
		CPPUNIT_ASSERT(pDefRoster->GetTranslatorInfo(translators[i],
			&outName, &outInfo, &outVersion) == B_OK);
		CPPUNIT_ASSERT(outName);
	}	
	delete[] translators;
	translators = NULL;
}

void
CheckTranslatorInfo(translator_info *pinfo, int32 nitems)
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

/**
 * Tests:
 * virtual status_t GetTranslators(BPositionIO *inSource,
 * BMessage *ioExtension, translator_info **outInfo, int32 *outNumInfo,
 * uint32 inHintType = 0, const char *inHintMIME = NULL, uint32 inWantType = 0) 
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::GetTranslatorsTest()
{
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	//open image to get a translator for
	BFile image("../src/tests/kits/translation/data/images/image.png",
		B_READ_ONLY);
	CPPUNIT_ASSERT(image.InitCheck() == B_OK);
	
	BFile garbled("../src/tests/kits/translation/data/garbled_data",
		B_READ_ONLY);
	CPPUNIT_ASSERT(garbled.InitCheck() == B_OK);
	
	NextSubTest();
	translator_info* pinfo = NULL;
	int32 outCount = -1;
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);

	//get translator, specifying wrong args
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->GetTranslators(&garbled, NULL, NULL,
		&outCount) == B_BAD_VALUE);

	//get translator, specifying wrong args
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->GetTranslators(&garbled, NULL, &pinfo,
		NULL) == B_BAD_VALUE);

	//get translator for garbled data
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->GetTranslators(&garbled, NULL, &pinfo,
		&outCount) == B_NO_TRANSLATOR);
		
	//get translator for image
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->GetTranslators(&image, NULL, &pinfo,
		&outCount) == B_OK);
	CPPUNIT_ASSERT(outCount > 0);
	CheckTranslatorInfo(pinfo, outCount);
	delete[] pinfo;
	pinfo = NULL;
	outCount = -1;
	
	//specify a valid BMessage
	NextSubTest();
	BMessage bmsg;
	CPPUNIT_ASSERT(
		bmsg.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, true) == B_OK);
	CPPUNIT_ASSERT(pDefRoster->GetTranslators(&image, &bmsg, &pinfo,
		&outCount, 0, NULL, B_TRANSLATOR_BITMAP) == B_OK);
	CPPUNIT_ASSERT(outCount > 0);
	CheckTranslatorInfo(pinfo, outCount);
	delete[] pinfo;
	pinfo = NULL;
}

/**
 * Tests:
 * virtual status_t Identify(BPositionIO *inSource, BMessage *ioExtension,
 * translator_info *outInfo, uint32 inHintType = 0,
 * const char *inHintMIME = NULL, uint32 inWantType = 0)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::IdentifyTest()
{
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	//open image to get a translator for
	BFile image("../src/tests/kits/translation/data/images/image.png",
		B_READ_ONLY);
	CPPUNIT_ASSERT(image.InitCheck() == B_OK);
	
	BFile garbled("../src/tests/kits/translation/data/garbled_data",
		B_READ_ONLY);
	CPPUNIT_ASSERT(garbled.InitCheck() == B_OK);

	NextSubTest();	
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);
	
	//get translator, specifying wrong args
	NextSubTest();	
	CPPUNIT_ASSERT(pDefRoster->Identify(&garbled, NULL, NULL) == B_BAD_VALUE);
	
	//get translator for garbled data
	NextSubTest();	
	CPPUNIT_ASSERT(pDefRoster->Identify(&garbled, NULL,
		&info) == B_NO_TRANSLATOR);
	
	//get translator for image
	NextSubTest();
	memset(&info, 0, sizeof(translator_info));
	CPPUNIT_ASSERT(pDefRoster->Identify(&image, NULL, &info) == B_OK);
	CheckTranslatorInfo(&info, 1);
	
	//supply desired output type
	NextSubTest();
	memset(&info, 0, sizeof(translator_info));
	CPPUNIT_ASSERT(pDefRoster->Identify(&image, NULL, &info,
		0, NULL, B_TRANSLATOR_BITMAP) == B_OK);
	CheckTranslatorInfo(&info, 1);
	
	// TODO: Add a test where I actually use pinfo and a BMessage 
	// TODO: and check their contents
}

/**
 * Tests:
 * virtual status_t MakeConfigurationView(translator_id forTranslator,
 * BMessage *ioExtension, BView **outView, BRect *outExtent)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::MakeConfigurationViewTest()
{
	//create invalid rect - if it is valid after the
	//MakeConfigurationView call the test has succeded
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	BRect extent(-1, -1, -1, -1);
	//create config view
	BView *view = NULL;
	translator_id *translators = NULL;
	int32 num_translators = -1;
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster);
	
	NextSubTest();
	CPPUNIT_ASSERT(
		pDefRoster->GetAllTranslators(&translators, &num_translators) == B_OK);
	CPPUNIT_ASSERT(translators);
	CPPUNIT_ASSERT(num_translators > 0);
	
	// bad parameters	
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->MakeConfigurationView(translators[0], NULL,
		&view, NULL) == B_BAD_VALUE);
	CPPUNIT_ASSERT(pDefRoster->MakeConfigurationView(translators[0], NULL,
		NULL, &extent) == B_BAD_VALUE);
	CPPUNIT_ASSERT(pDefRoster->MakeConfigurationView(translators[0], NULL,
		NULL, NULL) == B_BAD_VALUE);
		
	// bad translator id
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->MakeConfigurationView(-1, NULL, &view,
		&extent) == B_NO_TRANSLATOR);
	
	// should work
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->MakeConfigurationView(translators[0], NULL,
		&view, &extent) == B_OK);
	CPPUNIT_ASSERT(extent.IsValid() == true);
	
	// TODO: Add a test that uses a valid BMessage with actual settings in it

	//clean up after ourselves
	delete[] translators;
	translators = NULL;
}

/**
 * Tests:
 * virtual status_t Translate(BPositionIO *inSource,
 * const translator_info *inInfo, BMessage *ioExtension,
 * BPositionIO *outDestination, uint32 inWantOutType,
 * uint32 inHintType = 0, const char *inHintMIME = NULL)
 *
 * virtual status_t Translate(translator_id inTranslator,
 * BPositionIO *inSource, BMessage *ioExtension,
 * BPositionIO *outDestination, uint32 inWantOutType)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void
TranslatorRosterTest::TranslateTest()
{
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	//input
	BFile input("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(input.InitCheck() == B_OK);

	//temp file for generic format
	NextSubTest();
	BFile temp("/tmp/TranslatorRosterTest.temp",
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(temp.InitCheck() == B_OK);
	
	//output file
	NextSubTest();
	BFile output("../src/tests/kits/translation/data/images/image.out.tga",
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(output.InitCheck() == B_OK);
	
	//get default translators
	NextSubTest();
	BTranslatorRoster *pDefRoster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(pDefRoster != NULL);
	
	//translate to generic
	NextSubTest();
	CPPUNIT_ASSERT(pDefRoster->Translate(&input, NULL, NULL, &temp,
		B_TRANSLATOR_BITMAP) == B_OK);
	
	//translate to specific
	NextSubTest();	
	CPPUNIT_ASSERT(pDefRoster->Translate(&temp, NULL, NULL, &output,
		B_TGA_FORMAT) == B_OK);
	
	// TODO: add Translate using ioExtension BMessage and whatever
	// TODO: the other NULL parameter is
}

int
main()
{
	//needed by MakeConfigurationView
	TranslatorRosterTest test;
	test.InitializeTest();
	test.ConstructorTest();
	test.DefaultTest();
	test.InstantiateTest();
	test.VersionTest();
	test.AddTranslatorsTest();
	test.ArchiveTest();
	
	return 0;
}
