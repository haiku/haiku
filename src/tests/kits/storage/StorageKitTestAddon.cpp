#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "AppFileInfoTest.h"
#include "DirectoryTest.h"
#include "EntryTest.h"
#include "FileTest.h"
#include "FindDirectoryTest.h"
#include "MimeSnifferTest.h"
#include "MimeTypeTest.h"
#include "NodeInfoTest.h"
#include "NodeTest.h"
#include "PathTest.h"
#include "QueryTest.h"
#include "ResourcesTest.h"
#include "ResourceStringsTest.h"
#include "SymLinkTest.h"
#include "VolumeTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Storage");

	// ##### Add test suites here #####
	suite->addTest("BAppFileInfo", AppFileInfoTest::Suite());
	suite->addTest("BDirectory", DirectoryTest::Suite());
	suite->addTest("BEntry", EntryTest::Suite());
	suite->addTest("BFile", FileTest::Suite());
	suite->addTest("BMimeType", MimeTypeTest::Suite());
	suite->addTest("BNode", NodeTest::Suite());
	suite->addTest("BNodeInfo", NodeInfoTest::Suite());
	suite->addTest("BPath", PathTest::Suite());
	suite->addTest("BQuery", QueryTest::Suite());
	suite->addTest("BResources", ResourcesTest::Suite());
	suite->addTest("BResourceStrings", ResourceStringsTest::Suite());
	suite->addTest("BSymLink", SymLinkTest::Suite());
	suite->addTest("BVolume", VolumeTest::Suite());
	suite->addTest("FindDirectory", FindDirectoryTest::Suite());
	suite->addTest("MimeSniffer", MimeSnifferTest::Suite());
	
	return suite;
}
