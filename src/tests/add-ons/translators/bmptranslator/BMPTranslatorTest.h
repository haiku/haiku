// BMPTranslatorTest.h

#ifndef BMP_TRANSLATOR_TEST_H
#define BMP_TRANSLATOR_TEST_H

#include <TestCase.h>
#include <TestShell.h>

#define BMP_MIME_STRING "image/x-bmp"
#define BITS_MIME_STRING "image/x-be-bitmap"

class CppUnit::Test;

class BMPTranslatorTest : public BTestCase {
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
	void BTranslatorBasicTest();
	void BTranslatorIdentifyErrorTest();
	void BTranslatorIdentifyTest();
#endif

	void DummyTest();
};

#endif	// BMP_TRANSLATOR_TEST_H
