/*****************************************************************************/
// OpenBeOS Translation Kit Test
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
#include "TranslatorRosterTest.h"
#include <stdio.h>
#include <Message.h>
#include <Archivable.h>
#include <File.h>
#include <Application.h>

/* cppunit framework */
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

/**
 * TestCase constructor - no work
 */
TranslatorRosterTest::TranslatorRosterTest(std::string name) : BTestCase(name) {
}

/**
 * Default destructor - no work
 */
TranslatorRosterTest::~TranslatorRosterTest() {
}

CppUnit::Test*
TranslatorRosterTest::Suite() {
	/* create our suite */
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("TranslatorRoster");
		
	/* add suckers */
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Initialize Test", &TranslatorRosterTest::InitializeTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Constructor Test", &TranslatorRosterTest::ConstructorTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Default Test", &TranslatorRosterTest::DefaultTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Instantiate Test", &TranslatorRosterTest::InstantiateTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Version Test", &TranslatorRosterTest::VersionTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::AddTranslators Test", &TranslatorRosterTest::AddTranslatorsTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Archive Test", &TranslatorRosterTest::ArchiveTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::GetAllTranslators Test", &TranslatorRosterTest::GetAllTranslatorsTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::GetConfigurationMessage Test", &TranslatorRosterTest::GetConfigurationMessageTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::GetInputFormats Test", &TranslatorRosterTest::GetInputFormatsTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::GetOutputFormats Test", &TranslatorRosterTest::GetOutputFormatsTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::GetTranslatorInfo Test", &TranslatorRosterTest::GetTranslatorInfoTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::GetTranslators Test", &TranslatorRosterTest::GetTranslatorsTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Identify Test", &TranslatorRosterTest::IdentifyTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::MakeConfigurationView Test", &TranslatorRosterTest::MakeConfigurationViewTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorRosterTest>("TranslatorRosterTest::Translate Test", &TranslatorRosterTest::TranslateTest));			

	return suite;
}

/**
 * Tries to aquire the default roster
 */
void TranslatorRosterTest::InitializeTest() {
	//aquire default roster
	roster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(roster != NULL);
	
	//print version information
	int32 outCurVersion;
	int32 outMinVersion;
	long inAppVersion;
	const char* info = roster->Version(&outCurVersion, &outMinVersion, inAppVersion);
	printf("Default TranslatorRoster aquired. Version: %s\n", info);
}

/**
 * Construct roster using different kinds of constructors
 */
void TranslatorRosterTest::ConstructorTest() {
	//shared instance of TranslatorRoster
	BTranslatorRoster* translator_roster;

	//Create TranslatorRoster using noargs constructor
	translator_roster = new BTranslatorRoster();

	CPPUNIT_ASSERT(translator_roster != NULL);

	delete translator_roster;
	
	//Create TranslatorRoster using BMessage constructor
	NextSubTest();
	BMessage translator_message;
	translator_message.AddString("be:translator_path", "/boot/home/config/add-ons/Translators");
	translator_roster = new BTranslatorRoster(&translator_message);

	CPPUNIT_ASSERT(translator_roster != NULL);

	delete translator_roster;
}

/**
 * Tests:
 * BTranslatorRoster *Default()
 */
void TranslatorRosterTest::DefaultTest() {
	//already done in Initialize - added for completeness sake
	BTranslatorRoster* translator_roster = BTranslatorRoster::Default();

	CPPUNIT_ASSERT(translator_roster != NULL);
}

/**
 * Tests:
 * BTranslatorRoster *Instantiate(BMessage *from)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::InstantiateTest() {
	//shared instance of TranslatorRoster
	BTranslatorRoster* translator_roster = NULL;

	//Create our BMessage
	BMessage translator_message;
	
	//create BTranslator using empty message (must return NULL)
	translator_roster = (BTranslatorRoster*) BTranslatorRoster::Instantiate(&translator_message);
	CPPUNIT_ASSERT(translator_roster == NULL);

	if(translator_roster != NULL){
		delete translator_roster;
	}
	
	//create roster from message
	/*
	translator_message.AddString("be:translator_path", "/boot/home/config/add-ons/Translators/");
	translator_roster = (BTranslatorRoster*) BTranslatorRoster::Instantiate(&translator_message);

	CPPUNIT_ASSERT(translator_roster != NULL);

	delete translator_roster;
	*/
}

/**
 * Tests:
 * const char *Version(int32 *outCurVersion, int32 *outMinVersion, int32 *inAppVersion = B_TRANSLATION_CURRENT_VERSION)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::VersionTest() {
	int32 outCurVersion;
	int32 outMinVersion;
	long inAppVersion;
	const char* info = roster->Version(&outCurVersion, &outMinVersion, inAppVersion);

	CPPUNIT_ASSERT(info != NULL);
}

/**
 * Tests:
 * virtual status_t AddTranslators(const char *load_path = NULL) 
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::AddTranslatorsTest() {
	//create basic translatorroster
	BTranslatorRoster* translator_roster = new BTranslatorRoster();
	
	//load wrong path (generate parse error)
	/*
	CPPUNIT_ASSERT(translator_roster->AddTranslators("?") == B_BAD_VALUE);
	*/
	
	//load correct path
	CPPUNIT_ASSERT(translator_roster->AddTranslators("/boot/home/config/add-ons/Translators/:/system/add-ons/Translators/") == B_OK);

	NextSubTest();
	int32 num_translators;
	translator_id* translators;
	translator_roster->GetAllTranslators(&translators, &num_translators);
	
	CPPUNIT_ASSERT(num_translators > 0);
	
	delete [] translators;
	
	//delete and create new, this time don't specify path
	NextSubTest();
	delete translator_roster;
	translator_roster = new BTranslatorRoster();
	translator_roster->AddTranslators();	
	translator_roster->GetAllTranslators(&translators, &num_translators);
	
	CPPUNIT_ASSERT(num_translators > 0);
	
	delete [] translators;
	delete translator_roster;
}

/**
 * Tests:
 * virtual status_t Archive(BMessage *into, bool deep = true) const
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::ArchiveTest() {
	//archive default, and count entries (must be more than 1!)
	BMessage translator_message;
	roster = BTranslatorRoster::Default();
	roster->Archive(&translator_message);
	uint32 type;	
	int32 count;

	CPPUNIT_ASSERT(translator_message.GetInfo("be:translator_path", &type, &count) == B_OK);
}

/**
 * Tests:
 * virtual status_t GetAllTranslators(translator_id **outList, int32 *outCount)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::GetAllTranslatorsTest() {
	int32 num_translators;
	translator_id* translators;
	roster = BTranslatorRoster::Default();	
	roster->GetAllTranslators(&translators, &num_translators);

	CPPUNIT_ASSERT(num_translators > 0);

	delete [] translators;
}

/**
 * Tests:
 * virtual status_t GetConfigurationMessage(translator_id forTranslator, BMessage *ioExtension)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::GetConfigurationMessageTest() {
	BMessage translator_message;
	roster = BTranslatorRoster::Default();


	//get id for a translator (just use the first one)
	unsigned long translatorid;
	int32 num_translators;
	translator_id* translators;
	roster->GetAllTranslators(&translators, &num_translators);
	translatorid = translators[0];
	delete [] translators;
	
	//get conf for invalid translator
	CPPUNIT_ASSERT(roster->GetConfigurationMessage(-1, &translator_message) == B_NO_TRANSLATOR);
	
	//get conf for invalid ioExtension (BMessage)
	NextSubTest();
	CPPUNIT_ASSERT(roster->GetConfigurationMessage(translatorid, NULL) == B_BAD_VALUE);
	
	//get config for actual translator
	NextSubTest();
	CPPUNIT_ASSERT(roster->GetConfigurationMessage(translatorid, &translator_message) == B_OK);
}

/**
 * Tests:
 * virtual status_t GetInputFormats(translator_id forTranslator, const translation_format **outFormats, int32 *outNumFormats) 
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::GetInputFormatsTest() {
	translator_id* translators;
	int32 num_translators;
	roster = BTranslatorRoster::Default();	
	roster->GetAllTranslators(&translators, &num_translators);
	CPPUNIT_ASSERT(num_translators > 0);
	
	NextSubTest();
	for (int32 i=0;i<num_translators;i++) {
		const translation_format *fmts;
		int32 num_fmts;
		roster->GetInputFormats(translators[i], &fmts, &num_fmts);
		CPPUNIT_ASSERT(num_fmts >= 0);
	}
	delete [] translators;
}

/**
 * Tests:
 * virtual status_t GetOutputFormats(translator_id forTranslator, const translation_format **outFormats, int32 *outNumFormats)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::GetOutputFormatsTest() {
	translator_id* translators;
	int32 num_translators;
	roster = BTranslatorRoster::Default();	
	roster->GetAllTranslators(&translators, &num_translators);
	CPPUNIT_ASSERT(num_translators > 0);
	
	NextSubTest();
	for (int32 i=0;i<num_translators;i++) {
		const translation_format *fmts;
		int32 num_fmts;
		roster->GetOutputFormats(translators[i], &fmts, &num_fmts);
		CPPUNIT_ASSERT(num_fmts >= 0);		
	}
	delete [] translators;
}

/**
 * Tests:
 * virtual status_t GetTranslatorInfo(translator_id forTranslator, const char **outName, const char **outInfo, int32 *outVersion)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::GetTranslatorInfoTest() {
	translator_id* translators;
	int32 num_translators;
	roster = BTranslatorRoster::Default();
	roster->GetAllTranslators(&translators, &num_translators);	
	for (int32 i=0;i<num_translators;i++) {
		const char* outName;
		const char* outInfo;
		int32 outVersion;
		
		CPPUNIT_ASSERT(roster->GetTranslatorInfo(-1, &outName, &outInfo, &outVersion) == B_NO_TRANSLATOR);
		
		NextSubTest();
		CPPUNIT_ASSERT(roster->GetTranslatorInfo(translators[i], &outName, &outInfo, &outVersion) == B_OK);
	}	
	delete [] translators;	
}

/**
 * Tests:
 * virtual status_t GetTranslators(BPositionIO *inSource, BMessage *ioExtension, translator_info **outInfo, int32 *outNumInfo, uint32 inHintType = 0, const char *inHintMIME = NULL, uint32 inWantType = 0) 
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::GetTranslatorsTest() {
	BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	//open image to get a translator for
	BFile image("../src/tests/kits/translation/data/images/image.png", B_READ_ONLY);
	CPPUNIT_ASSERT(image.InitCheck() == B_OK);

	NextSubTest();
	BFile garbled("../src/tests/kits/translation/data/garbled_data", B_READ_ONLY);
	CPPUNIT_ASSERT(garbled.InitCheck() == B_OK);
	
	translator_info* info;
	int32 outCount;
	roster = BTranslatorRoster::Default();

	//get translator, specifying wrong args
	NextSubTest();
	CPPUNIT_ASSERT(roster->GetTranslators(&garbled, NULL, NULL, &outCount) == B_BAD_VALUE);

	//get translator, specifying wrong args
	NextSubTest();
	CPPUNIT_ASSERT(roster->GetTranslators(&garbled, NULL, &info, NULL) == B_BAD_VALUE);

	//get translator for garbled data
	NextSubTest();
	CPPUNIT_ASSERT(roster->GetTranslators(&garbled, NULL, &info, &outCount) == B_NO_TRANSLATOR);
		
	//get translator for image
	NextSubTest();
	CPPUNIT_ASSERT(roster->GetTranslators(&image, NULL, &info, &outCount) == B_OK);
	
	NextSubTest();

	CPPUNIT_ASSERT(outCount > 0);

	delete [] info;
}

/**
 * Tests:
 * virtual status_t Identify(BPositionIO *inSource, BMessage *ioExtension, translator_info *outInfo, uint32 inHintType = 0, const char *inHintMIME = NULL, uint32 inWantType = 0)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::IdentifyTest() {
	BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	//open image to get a translator for
	BFile image("../src/tests/kits/translation/data/images/image.png", B_READ_ONLY);
	CPPUNIT_ASSERT(image.InitCheck() == B_OK);

	//NextSubTest();
	BFile garbled("../src/tests/kits/translation/data/garbled_data", B_READ_ONLY);
	CPPUNIT_ASSERT(garbled.InitCheck() == B_OK);
	
	translator_info* info = new translator_info;
	roster = BTranslatorRoster::Default();
	
	//get translator, specifying wrong args
	NextSubTest();	
	CPPUNIT_ASSERT(roster->Identify(&garbled, NULL, NULL) == B_BAD_VALUE);
	
	//get translator for garbled data
	NextSubTest();	
	CPPUNIT_ASSERT(roster->Identify(&garbled, NULL, info) == B_NO_TRANSLATOR);
	
	//get translator for image
	NextSubTest();
	delete info;
	info = new translator_info;
	CPPUNIT_ASSERT(roster->Identify(&image, NULL, info) == B_OK);
	
	delete info;
}

/**
 * Tests:
 * virtual status_t MakeConfigurationView(translator_id forTranslator, BMessage *ioExtension, BView **outView, BRect *outExtent)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::MakeConfigurationViewTest() {
	//create invalid rect - if it is valid after the
	//MakeConfigurationView call the test has succeded
	BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	BRect extent(-1, -1, -1, -1);
	
	//create config view
	BView* view;
	translator_id* translators;
	int32 num_translators;
	roster = BTranslatorRoster::Default();
	roster->GetAllTranslators(&translators, &num_translators);	
	roster->MakeConfigurationView(translators[0], NULL, &view, &extent);

	//check validity
	CPPUNIT_ASSERT(extent.IsValid() == true);

	//clean up after ourselves
	delete [] translators;
}

/**
 * Tests:
 * virtual status_t Translate(BPositionIO *inSource, const translator_info *inInfo, BMessage *ioExtension, BPositionIO *outDestination, uint32 inWantOutType, uint32 inHintType = 0, const char *inHintMIME = NULL)
 * virtual status_t Translate(translator_id inTranslator, BPositionIO *inSource, BMessage *ioExtension, BPositionIO *outDestination, uint32 inWantOutType)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
void TranslatorRosterTest::TranslateTest() {
	BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	//input
	NextSubTest();
	BFile input("../src/tests/kits/translation/data/images/image.jpg", B_READ_ONLY);
	CPPUNIT_ASSERT(input.InitCheck() == B_OK);

	//temp file for generic format
	NextSubTest();
	BFile temp("/tmp/TranslatorRosterTest.temp",
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(temp.InitCheck() == B_OK);
	
	//output file
	NextSubTest();
	BFile output("../src/tests/kits/translation/data/images/image.out.png",
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(output.InitCheck() == B_OK);
	
	//get default translators
	NextSubTest();
	roster = BTranslatorRoster::Default();
	CPPUNIT_ASSERT(roster != NULL);
	
	//translate to generic
	NextSubTest();
	CPPUNIT_ASSERT(roster->Translate(&input, NULL, NULL, &temp, B_TRANSLATOR_BITMAP) == B_OK);
	
	//translate to specific
	NextSubTest();	
	CPPUNIT_ASSERT(roster->Translate(&temp, NULL, NULL, &output, B_PNG_FORMAT) == B_OK);
}

int main() {
	//needed by MakeConfigurationView
	TranslatorRosterTest test;
	test.InitializeTest();
	test.ConstructorTest();
	test.DefaultTest();
	test.InstantiateTest();
	test.VersionTest();
	test.AddTranslatorsTest();
	test.ArchiveTest();
}
