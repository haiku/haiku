// STXTTranslatorTest.h

#ifndef STXT_TRANSLATOR_TEST_H
#define STXT_TRANSLATOR_TEST_H

#include <TestCase.h>
#include <TestShell.h>

#define TEXT_MIME_STRING "text/plain"
#define STXT_MIME_STRING "text/x-vnd.Be-stxt"

class CppUnit::Test;

class STXTTranslatorTest : public BTestCase {
public:
	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
#if !TEST_R5
	void LoadAddOnTest();
#endif
	void IdentifyTest();
	void TranslateTest();
};

#endif	// STXT_TRANSLATOR_TEST_H
