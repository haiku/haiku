// STXTTranslatorTest.h

#ifndef STXT_TRANSLATOR_TEST_H
#define STXT_TRANSLATOR_TEST_H

#include <TestCase.h>
#include <TestShell.h>

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

#endif

	void DummyTest();
};

#endif	// STXT_TRANSLATOR_TEST_H
