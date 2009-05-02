// PNGTranslatorTest.h

#ifndef PNG_TRANSLATOR_TEST_H
#define PNG_TRANSLATOR_TEST_H

#include <TestCase.h>
#include <TestShell.h>

#define BBT_MIME_STRING  "image/x-be-bitmap"
#define PNG_MIME_STRING "image/png"

namespace CppUnit {
class Test;
}

class PNGTranslatorTest : public BTestCase {
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

#endif	// PNG_TRANSLATOR_TEST_H
