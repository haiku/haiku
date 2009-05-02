// TGATranslatorTest.h

#ifndef TGA_TRANSLATOR_TEST_H
#define TGA_TRANSLATOR_TEST_H

#include <TestCase.h>
#include <TestShell.h>

#define BBT_MIME_STRING  "image/x-be-bitmap"
#define TGA_MIME_STRING "image/x-targa"

namespace CppUnit {
class Test;
}

class TGATranslatorTest : public BTestCase {
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
	void ConfigMessageTest();
};

#endif	// TGA_TRANSLATOR_TEST_H
