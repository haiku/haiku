#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "BMPTranslatorTest.h"

BTestSuite *
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("Translators");

	// ##### Add test suites here #####
	suite->addTest("BMPTranslator", BMPTranslatorTest::Suite());
	
	return suite;
}
