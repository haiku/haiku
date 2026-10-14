/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 */

#include <Application.h>
#include <OS.h>
#include <Translator.h>
#include <TranslatorRoster.h>

#include <stdio.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


// BTranslator derived class to test with
class BTranslatorTester : public BTranslator {
public:
	BTranslatorTester();

	virtual const char* TranslatorName() const { return "NoName"; }
	virtual const char* TranslatorInfo() const { return "NoInfo"; }
	virtual int32 TranslatorVersion() const { return 100; }

	const translation_format* OutputFormats(int32* out_count) const
	{
		if (out_count)
			*out_count = 0;
		return NULL;
	}

	const translation_format* InputFormats(int32* out_count) const
	{
		if (out_count)
			*out_count = 0;
		return NULL;
	}

	virtual status_t Identify(BPositionIO* inSource, const translation_format* inFormat,
							  BMessage* ioExtension, translator_info* outInfo, uint32 outType)
	{
		return B_ERROR;
	}

	virtual status_t Translate(BPositionIO* inSource, const translator_info* inInfo,
							   BMessage* ioExtension, uint32 outType, BPositionIO* outDestination)
	{
		return B_ERROR;
	}
};

BTranslatorTester::BTranslatorTester()
	: BTranslator()
{
}

class TranslatorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TranslatorTest);
	CPPUNIT_TEST(InitialState_ReferenceCount_ReturnsOne);
	CPPUNIT_TEST(BaseImplementation_Acquire_IncrementsReferenceCount);
	CPPUNIT_TEST(BaseImplementation_Release_DecrementsReferenceCount);
	CPPUNIT_TEST(BaseImplementation_FinalRelease_DestroysObject);
	CPPUNIT_TEST(BaseImplementation_MakeConfigurationView_ReturnsError);
	CPPUNIT_TEST(BaseImplementation_GetConfigurationMessage_ReturnsError);
	CPPUNIT_TEST_SUITE_END();

public:
	void InitialState_ReferenceCount_ReturnsOne()
	{
		BTranslatorTester* ptt = new BTranslatorTester();
		CPPUNIT_ASSERT(ptt);
		CPPUNIT_ASSERT(ptt->ReferenceCount() == 1);
		ptt->Release();
	}

	void BaseImplementation_Acquire_IncrementsReferenceCount()
	{
		BTranslatorTester* ptt = new BTranslatorTester();
		BTranslator* ptranslator = static_cast<BTranslator*>(ptt);
		BTranslator* pcomptran = ptranslator->Acquire();
		CPPUNIT_ASSERT(pcomptran == ptranslator);
		CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 2);
		ptranslator->Release();
		ptranslator->Release();
	}

	void BaseImplementation_Release_DecrementsReferenceCount()
	{
		BTranslatorTester* ptt = new BTranslatorTester();
		BTranslator* ptranslator = static_cast<BTranslator*>(ptt);
		ptranslator->Acquire();
		CPPUNIT_ASSERT(ptranslator->Release() == ptranslator);
		CPPUNIT_ASSERT(ptranslator->ReferenceCount() == 1);
		ptranslator->Release();
	}

	void BaseImplementation_FinalRelease_DestroysObject()
	{
		BTranslatorTester* ptt = new BTranslatorTester();
		BTranslator* ptranslator = static_cast<BTranslator*>(ptt);
		CPPUNIT_ASSERT(ptranslator->Release() == NULL);
	}

	void BaseImplementation_MakeConfigurationView_ReturnsError()
	{
		BApplication app("application/x-vnd.OpenBeOS-translationkit_translatortest");
		BTranslatorTester* ptt = new BTranslatorTester();
		BTranslator* ptranslator = static_cast<BTranslator*>(ptt);

		BMessage bmsg;
		BView* pview = NULL;
		BRect rect;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, ptranslator->MakeConfigurationView(NULL, NULL, NULL));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, ptranslator->MakeConfigurationView(&bmsg, &pview, &rect));
		CPPUNIT_ASSERT(bmsg.IsEmpty() == true);

		ptranslator->Release();
	}

	void BaseImplementation_GetConfigurationMessage_ReturnsError()
	{
		BTranslatorTester* ptt = new BTranslatorTester();
		BTranslator* ptranslator = static_cast<BTranslator*>(ptt);

		BMessage bmsg;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, ptranslator->GetConfigurationMessage(NULL));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, ptranslator->GetConfigurationMessage(&bmsg));
		CPPUNIT_ASSERT(bmsg.IsEmpty() == true);

		ptranslator->Release();
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TranslatorTest, getTestSuiteName());
