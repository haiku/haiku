#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "generic_number_format/GenericNumberFormatTests.h"

// getTestSuite
BTestSuite*
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("NumberFormat");

	suite->addTest("BGenericNumberFormat", GenericNumberFormatTestSuite());

	return suite;
}
