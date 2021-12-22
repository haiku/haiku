/*****************************************************************************/
// Haiku Translation Kit Test
// Author: Michael Wilber
// Version:
//
// This is the Test application for BTranslator
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
#include "TranslatorTest.h"
#include <TranslatorRoster.h>
#include <Application.h>
#include <OS.h>
#include <stdio.h>

/* cppunit framework */
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

// BTranslator derived class to test with
class BTranslatorTester : public BTranslator {
public:
	BTranslatorTester();
	
	virtual const char *TranslatorName() const { return "NoName"; };
	virtual const char *TranslatorInfo() const { return "NoInfo"; };
	virtual int32 TranslatorVersion() const { return 100; };
	
	virtual const translation_format *InputFormats(int32 *out_count) const;
	virtual const translation_format *OutputFormats(int32 *out_count) const;

	virtual status_t Identify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType) { return B_ERROR; };

	virtual status_t Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination) { return B_ERROR; };
};

BTranslatorTester::BTranslatorTester()
	: BTranslator()
{
}

const translation_format *
BTranslatorTester::InputFormats(int32 *out_count) const
{
	if (out_count) *out_count = 0;
	return NULL;
}

const translation_format *
BTranslatorTester::OutputFormats(int32 *out_count) const
{
	if (out_count) *out_count = 0;
	return NULL;
}

/**
 * Default constructor - no work
 */
TranslatorTest::TranslatorTest(std::string name)
	: BTestCase(name)
{
}

/**
 * Default destructor - no work
 */
TranslatorTest::~TranslatorTest()
{
}

CppUnit::Test *
TranslatorTest::Suite()
{
	/* create our suite */
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("Translator");
		
	/* add suckers */
	suite->addTest(new CppUnit::TestCaller<TranslatorTest>("TranslatorTest::AcquireRelease Test", &TranslatorTest::AcquireReleaseTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorTest>("TranslatorTest::MakeConfigurationView Test", &TranslatorTest::MakeConfigurationViewTest));
	suite->addTest(new CppUnit::TestCaller<TranslatorTest>("TranslatorTest::GetConfigurationMessage Test", &TranslatorTest::GetConfigurationMessageTest));

	return suite;
}

void
TranslatorTest::AcquireReleaseTest()
{
	// Create new BTranslator
	NextSubTest();
	BTranslatorTester *ptt = new BTranslatorTester();
	CPPUNIT_ASSERT(ptt);
	BTranslator *ptranslator = static_cast<BTranslator *>(ptt);
	CPPUNIT_ASSERT(ptranslator);
	CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 1);
	
	// Do some Acquire()ing
	NextSubTest();
	BTranslator *pcomptran;
	pcomptran = ptranslator->Acquire();
	CPPUNIT_ASSERT(pcomptran == ptranslator);
	CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 2);
	
	// Release()
	NextSubTest();
	CPPUNIT_ASSERT(ptranslator->Release() == ptranslator);
	CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 1);
	
	// Destroy
	NextSubTest();
	CPPUNIT_ASSERT(ptranslator->Release() == NULL);
	ptranslator = NULL;
	ptt = NULL;
	pcomptran = NULL;
}

void
TranslatorTest::MakeConfigurationViewTest()
{
	// Create new BTranslator
	NextSubTest();
	BApplication app(
		"application/x-vnd.OpenBeOS-translationkit_translatortest");
	BTranslatorTester *ptt = new BTranslatorTester();
	CPPUNIT_ASSERT(ptt);
	BTranslator *ptranslator = static_cast<BTranslator *>(ptt);
	CPPUNIT_ASSERT(ptranslator);
	CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 1);
	
	// Try GetConfigurationMessage
	NextSubTest();
	BMessage bmsg;
	BView *pview = NULL;
	BRect rect;
	CPPUNIT_ASSERT(ptranslator->MakeConfigurationView(NULL,
		NULL, NULL) == B_ERROR);
	CPPUNIT_ASSERT(ptranslator->MakeConfigurationView(&bmsg,
		&pview, &rect) == B_ERROR);
	CPPUNIT_ASSERT(bmsg.IsEmpty() == true);
	
	// Destroy
	NextSubTest();
	CPPUNIT_ASSERT(ptranslator->Release() == NULL);
	ptranslator = NULL;
	ptt = NULL;
}

void
TranslatorTest::GetConfigurationMessageTest()
{
	// Create new BTranslator
	NextSubTest();
	BTranslatorTester *ptt = new BTranslatorTester();
	CPPUNIT_ASSERT(ptt);
	BTranslator *ptranslator = static_cast<BTranslator *>(ptt);
	CPPUNIT_ASSERT(ptranslator);
	CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 1);
	
	// Try GetConfigurationMessage
	NextSubTest();
	BMessage bmsg;
	CPPUNIT_ASSERT(ptranslator->GetConfigurationMessage(NULL) == B_ERROR);
	CPPUNIT_ASSERT(ptranslator->GetConfigurationMessage(&bmsg) == B_ERROR);
	CPPUNIT_ASSERT(bmsg.IsEmpty() == true);
	
	// Destroy
	NextSubTest();
	CPPUNIT_ASSERT(ptranslator->Release() == NULL);
	ptranslator = NULL;
	ptt = NULL;
}

