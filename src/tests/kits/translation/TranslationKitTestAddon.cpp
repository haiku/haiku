#include <TestSuite.h>
#include <TestSuiteAddon.h>
#include "TranslatorRosterTest.h"
#include "BitmapStreamTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("TranslatorRosterTest suite");
	suite->addTest("TranslatorRosterTest", TranslatorRosterTest::Suite());
	suite->addTest("BitmapStreamTest", BitmapStreamTest::Suite());
	return suite;
}
