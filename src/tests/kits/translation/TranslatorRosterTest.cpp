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

/**
 * Default constructor - no work
 */
TranslatorRosterTest::TranslatorRosterTest() {
}

/**
 * Default destructor - no work
 */
TranslatorRosterTest::~TranslatorRosterTest() {
}

/**
 * Initializes the test
 *
 * @return B_OK if initialization went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::Initialize() {
	//aquire default roster
	roster = BTranslatorRoster::Default();
	if(roster == NULL) {
		Debug("Failed to create aquire default TranslatorRoster\n");
		return B_ERROR;		
	}
	
	//print version information
	if(verbose && roster != NULL) {
		int32 outCurVersion;
		int32 outMinVersion;
		long inAppVersion;
		const char* info = roster->Version(&outCurVersion, &outMinVersion, inAppVersion);
		printf("Default TranslatorRoster aquired. Version: %s\n", info);
	}

	return B_OK;
}

/**
 * Initializes the tests.
 * This will test ALL methods in BTranslatorRoster.
 *
 * @return B_OK if the whole test went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::Perform() {
	if(ConstructorTest() != B_OK) {
		Debug("ERROR: ConstructorTest did not complete successfully\n");
		return B_ERROR;
	}
	if(DefaultTest() != B_OK) {
		Debug("ERROR: DefaultTest did not complete successfully\n");
		return B_ERROR;
	}
	if(InstantiateTest() != B_OK) {
		Debug("ERROR: InstantiateTest did not complete successfully\n");
		return B_ERROR;
	}
	if(VersionTest() != B_OK) {
		Debug("ERROR: VersionTest did not complete successfully\n");
		return B_ERROR;
	}
	if(AddTranslatorsTest() != B_OK) {
		Debug("ERROR: AddTranslatorsTest did not complete successfully\n");
		return B_ERROR;
	}
	if(ArchiveTest() != B_OK) {
		Debug("ERROR: ArchiveTest did not complete successfully\n");
		return B_ERROR;
	}	
	if(GetAllTranslatorsTest() != B_OK) {
		Debug("ERROR: GetAllTranslatorsTest did not complete successfully\n");
		return B_ERROR;
	}
	if(GetConfigurationMessageTest() != B_OK) {
		Debug("ERROR: GetConfigurationMessageTest did not complete successfully\n");
		return B_ERROR;
	}
	if(GetInputFormatsTest() != B_OK) {
		Debug("ERROR: GetInputFormatsTest did not complete successfully\n");
		return B_ERROR;
	}
	if(GetOutputFormatsTest() != B_OK) {
		Debug("ERROR: GetOutputFormatsTest did not complete successfully\n");
		return B_ERROR;
	}
	if(GetTranslatorInfoTest() != B_OK) {
		Debug("ERROR: GetTranslatorInfoTest did not complete successfully\n");
		return B_ERROR;
	}
	if(GetTranslatorsTest() != B_OK) {
		Debug("ERROR: GetTranslatorsTest did not complete successfully\n");
		return B_ERROR;
	}
	if(IdentifyTest() != B_OK) {
		Debug("ERROR: IdentifyTest did not complete successfully\n");
		return B_ERROR;
	}
	if(MakeConfigurationViewTest() != B_OK) {
		Debug("ERROR: MakeConfigurationViewTest did not complete successfully\n");
		return B_ERROR;
	}		
	if(TranslateTest() != B_OK) {
		Debug("ERROR: TranslateTest did not complete successfully\n");
		return B_ERROR;
	}
	
	return B_OK;
}
/**
 * Tests:
 * BTranslatorRoster()
 * BTranslatorRoster(BMessage *model)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::ConstructorTest() {
	//shared instance of TranslatorRoster
	BTranslatorRoster* translator_roster;	

	//Create TranslatorRoster using noargs constructor
	translator_roster = new BTranslatorRoster();
	if(translator_roster == NULL) {
		Debug("Could not create noargs BTranalatorRoster");
		return B_ERROR;
	}
	delete translator_roster;
	
	//Create TranslatorRoster using BMessage constructor
	BMessage translator_message;
	translator_message.AddString("be:translator_path", "/boot/home/config/add-ons/Translators");
	translator_roster = new BTranslatorRoster(&translator_message);
	if(translator_roster == NULL) {
		Debug("Could not create BMessage BTranalatorRoster");
		return B_ERROR;
	}
	delete translator_roster;
	
	return B_OK;
}

/**
 * Tests:
 * BTranslatorRoster *Default()
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::DefaultTest() {
	//already done in Initialize - added for completeness sake
	BTranslatorRoster* translator_roster = BTranslatorRoster::Default();
	if(translator_roster == NULL) {
		Debug("Unable to aquire default TranslatorRoster");
		return B_ERROR;
	}
	return B_OK;
}

/**
 * Tests:
 * BTranslatorRoster *Instantiate(BMessage *from)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::InstantiateTest() {
	//shared instance of TranslatorRoster
	BTranslatorRoster* translator_roster = NULL;

	//Create our BMessage
	BMessage translator_message;
	
	//create BTranslator using empty message (must return NULL)
	translator_roster = (BTranslatorRoster*) BTranslatorRoster::Instantiate(&translator_message);
	if(translator_roster != NULL) {
		Debug("Expected NULL when instantiating roster from invalid BMessage\n");
		delete translator_roster;
		return B_ERROR;
	}
	
	//create roster from message
	/*translator_message.AddString("be:translator_path", "/boot/home/config/add-ons/Translators/");
	translator_roster = (BTranslatorRoster*) BTranslatorRoster::Instantiate(&translator_message);
	if(translator_roster == NULL) {
		Debug("Could not instantiate BTranslatorRoster from archived default\n");
		printf("===== BMessage =====\n");
		translator_message.PrintToStream();
		printf("===== BMessage =====\n");
		return B_ERROR;
	}
	delete translator_roster;
	*/
	return B_OK;
}

/**
 * Tests:
 * const char *Version(int32 *outCurVersion, int32 *outMinVersion, int32 *inAppVersion = B_TRANSLATION_CURRENT_VERSION)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::VersionTest() {
	int32 outCurVersion;
	int32 outMinVersion;
	long inAppVersion;
	const char* info = roster->Version(&outCurVersion, &outMinVersion, inAppVersion);
	if(info == NULL) {
		Debug("Unable to obtain version information");
		return B_ERROR;
	}
	return B_OK;
}

/**
 * Tests:
 * virtual status_t AddTranslators(const char *load_path = NULL) 
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::AddTranslatorsTest() {
	//create basic translatorroster
	BTranslatorRoster* translator_roster = new BTranslatorRoster();
	
	//load wrong path (generate parse error)
	/*
	if(translator_roster->AddTranslators("?") != B_BAD_VALUE) {
		Debug("Expected B_BAD_VALUE when loading wrong path\n");
		return B_ERROR;	
	}
	*/
	
	//load correct path
	if(translator_roster->AddTranslators("/boot/home/config/add-ons/Translators/:/system/add-ons/Translators/") != B_OK) {
		Debug("Expected B_OK when loading addons from correct path\n");
		return B_ERROR;
	}

	int32 num_translators;
	translator_id* translators;
	translator_roster->GetAllTranslators(&translators, &num_translators);
	if(num_translators <= 0) {
		Debug("Unable to load translators, or no translators installed");
		return B_ERROR;
	}
	delete [] translators;
	
	//delete and create new, this time don't specify path
	delete translator_roster;
	translator_roster = new BTranslatorRoster();
	translator_roster->AddTranslators();	
	translator_roster->GetAllTranslators(&translators, &num_translators);
	if(num_translators <= 0) {
		Debug("Unable to load translators, or no translators installed");
		return B_ERROR;
	}
	delete [] translators;
	delete translator_roster;
		
	return B_OK;
}

/**
 * Tests:
 * virtual status_t Archive(BMessage *into, bool deep = true) const
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::ArchiveTest() {
	//archive default, and count entries (must be more than 1!)
	BMessage translator_message;
	roster->Archive(&translator_message);
	uint32 type;
	int32 count;
	if(translator_message.GetInfo("be:translator_path", &type, &count)) {
		Debug("Unable to retrieve archived information");
		return B_ERROR;
	}
	return B_OK;	
}

/**
 * Tests:
 * virtual status_t GetAllTranslators(translator_id **outList, int32 *outCount)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::GetAllTranslatorsTest() {
	int32 num_translators;
	translator_id* translators;
	roster->GetAllTranslators(&translators, &num_translators);
	if(num_translators <= 0) {
		Debug("Unable to load translators, or no translators installed");
		return B_ERROR;
	}
	delete [] translators;
		
	return B_OK;
}

/**
 * Tests:
 * virtual status_t GetConfigurationMessage(translator_id forTranslator, BMessage *ioExtension)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::GetConfigurationMessageTest() {
	BMessage translator_message;

	//get id for a translator (just use the first one)
	unsigned long translatorid;
	int32 num_translators;
	translator_id* translators;
	roster->GetAllTranslators(&translators, &num_translators);
	translatorid = translators[0];
	delete [] translators;
	
	//get conf for invalid translator
	if(roster->GetConfigurationMessage(-1, &translator_message) != B_NO_TRANSLATOR) {
		Debug("Expected B_ERROR when supplying wrong translator for GetConfigurationMessage\n");
		return B_ERROR;
	}
	
	//get conf for invalid ioExtension (BMessage)
	if(roster->GetConfigurationMessage(translatorid, NULL) != B_BAD_VALUE) {
		Debug("Expected B_BAD_VALUE when supplying wrong ioExtension for GetConfigurationMessage\n");
		return B_ERROR;
	}
	
	//get config for actual translator
	if(roster->GetConfigurationMessage(translatorid, &translator_message) != B_OK) {
		Debug("Could not get configuration for translator\n");
		return B_ERROR;
	}
	return B_OK;
}

/**
 * Tests:
 * virtual status_t GetInputFormats(translator_id forTranslator, const translation_format **outFormats, int32 *outNumFormats) 
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::GetInputFormatsTest() {
	translator_id* translators;
	int32 num_translators;
	roster->GetAllTranslators(&translators, &num_translators);
	if(num_translators <= 0) {
		Debug("Unable to load translators, or no translators installed");
		return B_ERROR;
	}
	
	for (int32 i=0;i<num_translators;i++) {
		const translation_format *fmts;
		int32 num_fmts;
		roster->GetInputFormats(translators[i], &fmts, &num_fmts);
		//printf("Translator supports %ld input formats\n", num_fmts);
		if(num_fmts <= 0) {
			Debug("Found translator accepting 0 or less input formats");
			return B_ERROR;
		}
	}
	delete [] translators;
	return B_OK;
}

/**
 * Tests:
 * virtual status_t GetOutputFormats(translator_id forTranslator, const translation_format **outFormats, int32 *outNumFormats)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::GetOutputFormatsTest() {
	translator_id* translators;
	int32 num_translators;
	roster->GetAllTranslators(&translators, &num_translators);
	if(num_translators <= 0) {
		Debug("Unable to load translators, or no translators installed");
		return B_ERROR;
	}
	
	for (int32 i=0;i<num_translators;i++) {
		const translation_format *fmts;
		int32 num_fmts;
		roster->GetOutputFormats(translators[i], &fmts, &num_fmts);
		//printf("Translator supports %ld output formats\n", num_fmts);
		if(num_fmts <= 0) {
			Debug("Found translator accepting 0 or less output formats");
			return B_ERROR;
		}
		
	}
	delete [] translators;
	return B_OK;
}

/**
 * Tests:
 * virtual status_t GetTranslatorInfo(translator_id forTranslator, const char **outName, const char **outInfo, int32 *outVersion)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::GetTranslatorInfoTest() {
	translator_id* translators;
	int32 num_translators;
	roster->GetAllTranslators(&translators, &num_translators);	
	for (int32 i=0;i<num_translators;i++) {
		const char* outName;
		const char* outInfo;
		int32 outVersion;
		if(roster->GetTranslatorInfo(-1, &outName, &outInfo, &outVersion) != B_NO_TRANSLATOR) {
			Debug("Expected B_NO_TRANSLATOR when supplying wrong id");
			return B_ERROR;
		}
		
		if(roster->GetTranslatorInfo(translators[i], &outName, &outInfo, &outVersion) != B_OK) {
			Debug("Unable to get Translator info");
			return B_ERROR;
		}
		//printf("Translator info:\nName: %s\nInfo: %s\nVersion: %ld\n", outName, outInfo, outVersion);		
	}	
	delete [] translators;	
	return B_OK;
}

/**
 * Tests:
 * virtual status_t GetTranslators(BPositionIO *inSource, BMessage *ioExtension, translator_info **outInfo, int32 *outNumInfo, uint32 inHintType = 0, const char *inHintMIME = NULL, uint32 inWantType = 0) 
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::GetTranslatorsTest() {
	//open image to get a translator for
	BFile image("./data/images/image.png", B_READ_ONLY);
	BFile garbled("./data/garbled_data", B_READ_ONLY);
	
	translator_info* info;
	int32 outCount;
	
	//get translator, specifying wrong args
	if(roster->GetTranslators(&garbled, NULL, NULL, &outCount) != B_BAD_VALUE) {
		Debug("Expected B_BAD_VALUE when getting translator using illegal arguments");
		return B_ERROR;
	}
	
	//get translator, specifying wrong args
	if(roster->GetTranslators(&garbled, NULL, &info, NULL) != B_BAD_VALUE) {
		Debug("Expected B_BAD_VALUE when getting translator using illegal arguments");
		return B_ERROR;
	}
	
	//get translator for garbled data
	if(roster->GetTranslators(&garbled, NULL, &info, &outCount) != B_NO_TRANSLATOR) {
		Debug("Expected B_NO_TRANSLATOR when getting translator for garbled data");
		return B_ERROR;
	}
		
	//get translator for image
	if(roster->GetTranslators(&image, NULL, &info, &outCount) != B_OK) {
		Debug("Expected B_OK when getting translator for image");
		return B_ERROR;
	}
	
	if(outCount <= 0) {
		Debug("Unable to find translator for image data");
		return B_ERROR;
	} 
	//else {
	//iterate through all translators
	//for(int32 i=0; i<outCount;i++) {
	//	printf("Found translator:\nType: %ld\nid: %ld\nGroup: %ld\nQuality: %f\nCapability: %f\nName: %s\nMIME: %s\n", info->type, info->translator, info->group, info->quality, info->capability, info->name, info->MIME);
	//}		
	//}
	delete [] info;
	
	return B_OK;
}

/**
 * Tests:
 * virtual status_t Identify(BPositionIO *inSource, BMessage *ioExtension, translator_info *outInfo, uint32 inHintType = 0, const char *inHintMIME = NULL, uint32 inWantType = 0)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::IdentifyTest() {
	//open image to get a translator for
	BFile image("./data/images/image.png", B_READ_ONLY);
	BFile garbled("./data/garbled_data", B_READ_ONLY);
	
	translator_info* info;
	
	//get translator, specifying wrong args
	if(roster->Identify(&garbled, NULL, NULL) != B_BAD_VALUE) {
		Debug("Expected B_BAD_VALUE when getting translator using illegal arguments");
		return B_ERROR;
	}
	
	//get translator for garbled data
	if(roster->Identify(&garbled, NULL, info) != B_NO_TRANSLATOR) {
		Debug("Expected B_NO_TRANSLATOR when getting translator for garbled data");
		return B_ERROR;
	}
		
	//get translator for image
	if(roster->Identify(&image, NULL, info) != B_OK) {
		Debug("Expected B_OK when getting translator for image");
		return B_ERROR;
	}
	return B_OK;
}

/**
 * Tests:
 * virtual status_t MakeConfigurationView(translator_id forTranslator, BMessage *ioExtension, BView **outView, BRect *outExtent)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::MakeConfigurationViewTest() {
	//create invalid rect - if it is valid after the
	//MakeConfigurationView call the test has succeded
	BRect extent(-1, -1, -1, -1);
	
	//create config view
	BView* view;
	translator_id* translators;
	int32 num_translators;
	roster->GetAllTranslators(&translators, &num_translators);	
	roster->MakeConfigurationView(translators[0], NULL, &view, &extent);

	//check validity
	bool success = (extent.IsValid()) ? B_OK : B_ERROR;

	//clean up after ourselves
	delete [] translators;
	
	return success;
}

/**
 * Tests:
 * virtual status_t Translate(BPositionIO *inSource, const translator_info *inInfo, BMessage *ioExtension, BPositionIO *outDestination, uint32 inWantOutType, uint32 inHintType = 0, const char *inHintMIME = NULL)
 * virtual status_t Translate(translator_id inTranslator, BPositionIO *inSource, BMessage *ioExtension, BPositionIO *outDestination, uint32 inWantOutType)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t TranslatorRosterTest::TranslateTest() {
	//input
	BFile input("./data/images/image.jpg", B_READ_ONLY);

	//temp file for generic format
	BFile temp("/tmp/TranslatorRosterTest.temp", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	
	//output file
	BFile output("./data/images/image.out.png", B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	
	//translate to generic
	if(roster->Translate(&input, NULL, NULL, &temp, B_TRANSLATOR_BITMAP) != B_OK) {
		Debug("Could not translate image to generic format");
		return B_ERROR;
	}
	
	//translate to specific
	if(roster->Translate(&temp, NULL, NULL, &output, B_PNG_FORMAT) != B_OK) {
		Debug("Could not translate generic image to specific format");
		return B_ERROR;
	}
	return B_OK;
}

/**
 * Prints debug information to stdout, if verbose is set to true
 */
void TranslatorRosterTest::Debug(char* string) {
	if(verbose) {
		printf(string);
	}
}

int main() {
	//needed by MakeConfigurationView
	BApplication app("application/x-vnd.OpenBeOS-translationkit_translatorrostertest");
	TranslatorRosterTest test;
	test.setVerbose(true);
	test.Initialize();
	if(test.Perform() != B_OK) {
		printf("Tests did not complete successfully\n");
	} else {
		printf("All tests completed without errors\n");
	}
}