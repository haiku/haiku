#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "bmptranslator/BMPTranslatorTest.h"
#include "stxttranslator/STXTTranslatorTest.h"

BTestSuite *
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("Translators");

	// ##### Add test suites here #####
	suite->addTest("BMPTranslator", BMPTranslatorTest::Suite());
	suite->addTest("STXTTranslator", STXTTranslatorTest::Suite());
	
	return suite;
}
