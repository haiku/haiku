// STXTTranslatorTest.cpp
#include "STXTTranslatorTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <image.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <Message.h>
#include <View.h>
#include <Rect.h>
#include <File.h>
#include <DataIO.h>
#include <Errors.h>
#include <OS.h>
#include "TranslatorTestAddOn.h"

// Suite
CppUnit::Test *
STXTTranslatorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<STXTTranslatorTest> TC;
			
	suite->addTest(
		new TC("STXTTranslator IdentifyTest",
			&STXTTranslatorTest::IdentifyTest));

	suite->addTest(
		new TC("STXTTranslator TranslateTest",
			&STXTTranslatorTest::TranslateTest));
			
	suite->addTest(
		new TC("STXTTranslator ConfigMessageTest",
			&STXTTranslatorTest::ConfigMessageTest));

#if !TEST_R5
	suite->addTest(
		new TC("STXTTranslator LoadAddOnTest",
			&STXTTranslatorTest::LoadAddOnTest));
#endif
		
	return suite;
}		

// setUp
void
STXTTranslatorTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
STXTTranslatorTest::tearDown()
{
	BTestCase::tearDown();
}

void
CheckStyled(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_STYLED_TEXT_FORMAT, B_TRANSLATOR_TEXT,
		0.5f, 0.5f, "Be styled text file", "text/x-vnd.Be-stxt");
}

void
CheckPlain(translator_info *pti)
{
	CheckTranslatorInfo(pti, B_TRANSLATOR_TEXT, B_TRANSLATOR_TEXT,
		0.4f, 0.6f, "Plain text file", "text/plain");
}

void
IdentifyTests(STXTTranslatorTest *ptest, BTranslatorRoster *proster,
	const char **paths, int32 len, bool bplain)
{
	translator_info ti;
	printf(" [%d] ", (int) bplain);
	
	for (int32 i = 0; i < len; i++) {
		ptest->NextSubTest();
		BFile file;
		printf(" [%s] ", paths[i]);
		CPPUNIT_ASSERT(file.SetTo(paths[i], B_READ_ONLY) == B_OK);

		// Identify (output: B_TRANSLATOR_ANY_TYPE)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti) == B_OK);
		if (bplain)
			CheckPlain(&ti);
		else
			CheckStyled(&ti);
	
		// Identify (output: B_TRANSLATOR_TEXT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_TRANSLATOR_TEXT) == B_OK);
		if (bplain)
			CheckPlain(&ti);
		else
			CheckStyled(&ti);
	
		// Identify (output: B_STYLED_TEXT_FORMAT)
		ptest->NextSubTest();
		memset(&ti, 0, sizeof(translator_info));
		CPPUNIT_ASSERT(proster->Identify(&file, NULL, &ti, 0, NULL,
			B_STYLED_TEXT_FORMAT) == B_OK);
		if (bplain)
			CheckPlain(&ti);
		else
			CheckStyled(&ti);
	}
}

void
STXTTranslatorTest::IdentifyTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/STXTTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
		
	// Identify (bad input, output types)
	NextSubTest();
	translator_info ti;
	memset(&ti, 0, sizeof(translator_info));
	result = proster->Identify(&wronginput, NULL, &ti, 0,
		NULL, B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(ti.type == 0 && ti.translator == 0);
		
	// Identify (wrong type of input data)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	result = proster->Identify(&wronginput, NULL, &ti);
	CPPUNIT_ASSERT(result == B_OK);
	CheckTranslatorInfo(&ti, B_TRANSLATOR_TEXT, B_TRANSLATOR_TEXT, 0.4f, 0.36f,
		"Plain text file", "text/plain");
	
	// Identify (wrong magic)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	BFile wrongmagic("../src/tests/kits/translation/data/text/wrong_magic.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wrongmagic.InitCheck() == B_OK);
	result = proster->Identify(&wrongmagic, NULL, &ti);
	CPPUNIT_ASSERT(result == B_OK);
	CheckTranslatorInfo(&ti, B_TRANSLATOR_TEXT, B_TRANSLATOR_TEXT, 0.4f, 0.36f,
		"Plain text file", "text/plain");
	
	// Identify (wrong version)
	NextSubTest();
	memset(&ti, 0, sizeof(translator_info));
	BFile wrongversion("../src/tests/kits/translation/data/text/wrong_version.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wrongversion.InitCheck() == B_OK);
	result = proster->Identify(&wrongversion, NULL, &ti);
	CPPUNIT_ASSERT(result == B_OK);
	CheckTranslatorInfo(&ti, B_TRANSLATOR_TEXT, B_TRANSLATOR_TEXT, 0.4f, 0.36f,
		"Plain text file", "text/plain");
	
	const char *aPlainFiles[] = {
		"../src/tests/kits/translation/data/text/ascii.txt",
		"../src/tests/kits/translation/data/text/japanese.txt",
		"../src/tests/kits/translation/data/text/multi_byte.txt",
		"../src/tests/kits/translation/data/text/one_length.txt",
		"../src/tests/kits/translation/data/text/sentence.txt",
		"../src/tests/kits/translation/data/text/symbols.txt",
		"../src/tests/kits/translation/data/text/zero_length.txt"
	};
	const char *aStyledFiles[] = {
		"../src/tests/kits/translation/data/text/ascii.stxt",
		"../src/tests/kits/translation/data/text/japanese.stxt",
		"../src/tests/kits/translation/data/text/multi_byte.stxt",
		"../src/tests/kits/translation/data/text/one_length.stxt",
		"../src/tests/kits/translation/data/text/sentence.stxt",
		"../src/tests/kits/translation/data/text/symbols.stxt",
		"../src/tests/kits/translation/data/text/zero_length.stxt",
	};
	IdentifyTests(this, proster, aPlainFiles,
		sizeof(aPlainFiles) / sizeof(const char *), true);
	IdentifyTests(this, proster, aStyledFiles,
		sizeof(aStyledFiles) / sizeof(const char *), false);
}

void
TranslateTests(STXTTranslatorTest *ptest, BTranslatorRoster *proster,
	const char **paths, int32 len, bool bplain)
{
	int32 nlongest = 0, ncurrent = 0;
	// find the length of the longest string
	for (int32 i = 0; i < len; i++) {
		ncurrent = strlen(paths[i]);
		if (ncurrent > nlongest)
			nlongest = ncurrent;
	}
	
	char *styled_path = NULL, *plain_path = NULL;
	styled_path = new char[nlongest + 6];
	plain_path = new char[nlongest + 6];
	
	// Perform translations on every file in the array
	for (int32 i = 0; i < len; i++) {
		strcpy(styled_path, paths[i]);
		strcat(styled_path, ".stxt");
		strcpy(plain_path, paths[i]);
		strcat(plain_path, ".txt");
		
		// Setup input files	
		ptest->NextSubTest();
		BFile styled_file, plain_file, *pinput_file;
		CPPUNIT_ASSERT(styled_file.SetTo(styled_path, B_READ_ONLY) == B_OK);
		CPPUNIT_ASSERT(plain_file.SetTo(plain_path, B_READ_ONLY) == B_OK);
		if (bplain) {
			printf(" [%s] ", plain_path);
			pinput_file = &plain_file;
		} else {
			printf(" [%s] ", styled_path);
			pinput_file = &styled_file;
		}
		
		BMallocIO mallio, dmallio;
		
		// Convert to B_TRANSLATOR_ANY_TYPE (should be B_TRANSLATOR_TEXT)
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput_file, NULL, NULL, &mallio,
			B_TRANSLATOR_ANY_TYPE) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, plain_file) == true);
		
		// Convert to B_TRANSLATOR_TEXT
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput_file, NULL, NULL, &mallio,
			B_TRANSLATOR_TEXT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, plain_file) == true);
		
		// Convert plain mallio to B_TRANSLATOR_TEXT dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_TEXT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, plain_file) == true);
		
		// Convert to B_STYLED_TEXT_FORMAT
		ptest->NextSubTest();
		CPPUNIT_ASSERT(mallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(mallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(pinput_file, NULL, NULL, &mallio,
			B_STYLED_TEXT_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(mallio, styled_file) == true);
		
		// Convert styled mallio to B_TRANSLATOR_TEXT dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_TRANSLATOR_TEXT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, plain_file) == true);
		
		// Convert styled mallio to B_STYLED_TEXT_FORMAT dmallio
		ptest->NextSubTest();
		CPPUNIT_ASSERT(dmallio.Seek(0, SEEK_SET) == 0);
		CPPUNIT_ASSERT(dmallio.SetSize(0) == B_OK);
		CPPUNIT_ASSERT(proster->Translate(&mallio, NULL, NULL, &dmallio,
			B_STYLED_TEXT_FORMAT) == B_OK);
		CPPUNIT_ASSERT(CompareStreams(dmallio, styled_file) == true);
	}
	
	delete[] styled_path;
	styled_path = NULL;
	delete[] plain_path;
	plain_path = NULL;
}

void
STXTTranslatorTest::TranslateTest()
{
	// Init
	NextSubTest();
	status_t result = B_ERROR;
	off_t filesize = -1;
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/STXTTranslator") == B_OK);
	BFile wronginput("../src/tests/kits/translation/data/images/image.jpg",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wronginput.InitCheck() == B_OK);
	BFile output("/tmp/stxt_test.out", B_READ_WRITE | 
		B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT(output.InitCheck() == B_OK);
	
	// Translate (bad input, output types)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_BITMAP);
	CPPUNIT_ASSERT(result == B_NO_TRANSLATOR);
	CPPUNIT_ASSERT(output.GetSize(&filesize) == B_OK);
	CPPUNIT_ASSERT(filesize == 0);
	
	// Translate (wrong type of input data)
	NextSubTest();
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_TEXT);
	CPPUNIT_ASSERT(result == B_OK);
	CPPUNIT_ASSERT(CompareStreams(wronginput, output) == true);
	
	// Translate (wrong type of input, B_TRANSLATOR_ANY_TYPE output)
	NextSubTest();
	CPPUNIT_ASSERT(output.Seek(0, SEEK_SET) == 0);
	CPPUNIT_ASSERT(output.SetSize(0) == B_OK);
	result = proster->Translate(&wronginput, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_OK);
	CPPUNIT_ASSERT(CompareStreams(wronginput, output) == true);
	
	// Translate (wrong magic)
	NextSubTest();
	CPPUNIT_ASSERT(output.Seek(0, SEEK_SET) == 0);
	CPPUNIT_ASSERT(output.SetSize(0) == B_OK);
	BFile wrongmagic("../src/tests/kits/translation/data/text/wrong_magic.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wrongmagic.InitCheck() == B_OK);
	result = proster->Translate(&wrongmagic, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_OK);
	CPPUNIT_ASSERT(CompareStreams(wrongmagic, output) == true);
	
	// Translate (wrong version)
	NextSubTest();
	CPPUNIT_ASSERT(output.Seek(0, SEEK_SET) == 0);
	CPPUNIT_ASSERT(output.SetSize(0) == B_OK);
	BFile wrongversion("../src/tests/kits/translation/data/text/wrong_version.stxt",
		B_READ_ONLY);
	CPPUNIT_ASSERT(wrongversion.InitCheck() == B_OK);
	result = proster->Translate(&wrongversion, NULL, NULL, &output,
		B_TRANSLATOR_ANY_TYPE);
	CPPUNIT_ASSERT(result == B_OK);
	CPPUNIT_ASSERT(CompareStreams(wrongversion, output) == true);
	
	// Translate various data
	const char *aPlainTextFiles[] = {
		"../src/tests/kits/translation/data/text/ascii",
		"../src/tests/kits/translation/data/text/japanese",
		"../src/tests/kits/translation/data/text/multi_byte",
		"../src/tests/kits/translation/data/text/one_length",
		"../src/tests/kits/translation/data/text/symbols",
		"../src/tests/kits/translation/data/text/zero_length"
	};
	const char *aStyledTextFiles[] = {
		"../src/tests/kits/translation/data/text/ascii",
		"../src/tests/kits/translation/data/text/japanese",
		"../src/tests/kits/translation/data/text/multi_byte",
		"../src/tests/kits/translation/data/text/one_length",
		"../src/tests/kits/translation/data/text/sentence",
		"../src/tests/kits/translation/data/text/symbols",
		"../src/tests/kits/translation/data/text/zero_length"
	};
	TranslateTests(this, proster, aPlainTextFiles,
		sizeof(aPlainTextFiles) / sizeof(const char *), true);
	TranslateTests(this, proster, aStyledTextFiles,
		sizeof(aStyledTextFiles) / sizeof(const char *), false);
}

// Make certain that the STXTTranslator does not
// provide a configuration message
void
STXTTranslatorTest::ConfigMessageTest()
{
	// Init
	NextSubTest();
	BTranslatorRoster *proster = new BTranslatorRoster();
	CPPUNIT_ASSERT(proster);
	CPPUNIT_ASSERT(proster->AddTranslators(
		"/boot/home/config/add-ons/Translators/STXTTranslator") == B_OK);
		
	// GetAllTranslators
	NextSubTest();
	translator_id tid, *pids = NULL;
	int32 count = 0;
	CPPUNIT_ASSERT(proster->GetAllTranslators(&pids, &count) == B_OK);
	CPPUNIT_ASSERT(pids);
	CPPUNIT_ASSERT(count == 1);
	tid = pids[0];
	delete[] pids;
	pids = NULL;
	
	// GetConfigurationMessage
	NextSubTest();
	BMessage msg;
	CPPUNIT_ASSERT(proster->GetConfigurationMessage(tid, &msg) == B_OK);
	CPPUNIT_ASSERT(!msg.IsEmpty());
	
	// B_TRANSLATOR_EXT_HEADER_ONLY
	NextSubTest();
	bool bheaderonly = true;
	CPPUNIT_ASSERT(
		msg.FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderonly) == B_OK);
	CPPUNIT_ASSERT(bheaderonly == false);
	
	// B_TRANSLATOR_EXT_DATA_ONLY
	NextSubTest();
	bool bdataonly = true;
	CPPUNIT_ASSERT(
		msg.FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataonly) == B_OK);
	CPPUNIT_ASSERT(bdataonly == false);
}

#if !TEST_R5

// The input formats that this translator is supposed to support
translation_format gSTXTInputFormats[] = {
	{
		B_TRANSLATOR_TEXT,
		B_TRANSLATOR_TEXT,
		0.4f, // quality
		0.6f, // capability
		"text/plain",
		"Plain text file"
	},
	{
		B_STYLED_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		0.5f,
		0.5f,
		"text/x-vnd.Be-stxt",
		"Be styled text file"
	}
};

// The output formats that this translator is supposed to support
translation_format gSTXTOutputFormats[] = {
	{
		B_TRANSLATOR_TEXT,
		B_TRANSLATOR_TEXT,
		0.4f, // quality
		0.6f, // capability
		"text/plain",
		"Plain text file"
	},
	{
		B_STYLED_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		0.5f,
		0.5f,
		"text/x-vnd.Be-stxt",
		"Be styled text file"
	}
};

void
STXTTranslatorTest::LoadAddOnTest()
{
	TranslatorLoadAddOnTest("/boot/home/config/add-ons/Translators/STXTTranslator",
		this,
		gSTXTInputFormats, sizeof(gSTXTInputFormats) / sizeof(translation_format),
		gSTXTOutputFormats, sizeof(gSTXTOutputFormats) / sizeof(translation_format),
		B_TRANSLATION_MAKE_VERSION(1,0,0));
}

#endif // #if !TEST_R5
