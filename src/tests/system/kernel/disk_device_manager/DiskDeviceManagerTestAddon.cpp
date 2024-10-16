#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "KPartitionTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("DiskDeviceManager");
	suite->addTest("KPartitionGetMountPointTest", KPartitionGetMountPointTest::Suite());
	return suite;
}
