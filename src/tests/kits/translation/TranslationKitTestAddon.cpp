#include <TestSuite.h>
#include <TestSuiteAddon.h>
#include "TranslatorRosterTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("TranslatorRosterTest suite");
	suite->addTest("TranslatorRosterTest", TranslatorRosterTest::Suite());
	return suite;
}
