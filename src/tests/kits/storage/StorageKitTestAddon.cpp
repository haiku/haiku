#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include <DirectoryTest.h>
#include <NodeTest.h>
#include <PathTest.h>

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Storage");

	// ##### Add test suites for statically linked tests here #####
	suite->addTest("BDirectory", DirectoryTest::Suite());
	suite->addTest("BNode", NodeTest::Suite());
	suite->addTest("BPath", PathTest::Suite());
	
	return suite;
}
