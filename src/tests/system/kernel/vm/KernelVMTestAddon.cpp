#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "VMGetMountPointTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("KernelVM");
	suite->addTest("VMGetMountPointTest", VMGetMountPointTest::Suite());
	return suite;
}
