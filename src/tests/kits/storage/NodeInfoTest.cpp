// NodeInfoTest.cpp

#include <stdio.h>
#include <string>
#include <unistd.h>

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TypeConstants.h>

#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "NodeInfoTest.h"
#include "../app/bmessenger/Helpers.h"

// test dirs/files/types
static const char *testDir			= "/tmp/testDir";
static const char *testFile1		= "/tmp/testDir/file1";
static const char *testFile2		= "/tmp/testDir/file2";
static const char *testType1		= "application/x-vnd.obos.node-info-test1";
static const char *testType2		= "application/x-vnd.obos.node-info-test2";
static const char *invalidTestType	= "invalid-mime-type";
static const char *tooLongTestType	=
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
;

// attributes
static const char *kTypeAttribute			= "BEOS:TYPE";
static const char *kMiniIconAttribute		= "BEOS:M:STD_ICON";
static const char *kLargeIconAttribute		= "BEOS:L:STD_ICON";
//static const char *kPreferredAppAttribute	= "BEOS:PREF_APP";
//static const char *kAppHintAttribute		= "BEOS:PPATH";

// Suite
CppUnit::Test*
NodeInfoTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<NodeInfoTest> TC;
		
//	suite->addTest( new TC("BNodeInfo::Init Test1", &NodeInfoTest::InitTest1) );
//	suite->addTest( new TC("BNodeInfo::Init Test2", &NodeInfoTest::InitTest2) );
	suite->addTest( new TC("BNodeInfo::Type Test", &NodeInfoTest::TypeTest) );
//	suite->addTest( new TC("BNodeInfo::Icon Test", &NodeInfoTest::IconTest) );
//	suite->addTest( new TC("BNodeInfo::Preferred App Test",
//						   &NodeInfoTest::PreferredAppTest) );
//	suite->addTest( new TC("BNodeInfo::App Hint Test",
//						   &NodeInfoTest::AppHintTest) );
//	suite->addTest( new TC("BNodeInfo::Tracker Icon Test",
//						   &NodeInfoTest::TrackerIconTest) );

	return suite;
}		

// setUp
void
NodeInfoTest::setUp()
{
	BasicTest::setUp();

	execCommand(
		string("mkdir ") + testDir
		+ "; touch " + testFile1
		+ "; touch " + testFile2
	);
	fApplication = new BApplication("application/x-vnd.obos.node-info-test");
}
	
// tearDown
void
NodeInfoTest::tearDown()
{
	// delete the application
	delete fApplication;
	fApplication = NULL;
	// remove the types we've added
	const char * const testTypes[] = {
		testType1, testType2
	};
	for (uint32 i = 0; i < sizeof(testTypes) / sizeof(const char*); i++) {
		BMimeType type(testTypes[i]);
		type.Delete();
	}

	// delete the test dir
	execCommand(string("rm -rf ") + testDir);

	BasicTest::tearDown();
}

// InitTest1
void
NodeInfoTest::InitTest1()
{
	// BNodeInfo()
	// * InitCheck() == B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		CHK(nodeInfo.InitCheck() == B_NO_INIT);
	}

	// BNodeInfo(BNode *node)
	// * NULL node => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BNodeInfo nodeInfo(NULL);
		CHK(nodeInfo.InitCheck() == B_BAD_VALUE);
	}
	// * invalid node => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BNode node;
		BNodeInfo nodeInfo(&node);
		CHK(nodeInfo.InitCheck() == B_BAD_VALUE);
	}
	// * valid node => InitCheck() == B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo(&node);
		CHK(nodeInfo.InitCheck() == B_OK);
	}
}

// InitTest2
void
NodeInfoTest::InitTest2()
{
	// status_t SetTo(BNode *node)
	// * NULL node => InitCheck() == B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(NULL) == B_BAD_VALUE);
		CHK(nodeInfo.InitCheck() == B_BAD_VALUE);
	}
	// * invalid node => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BNode node;
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_BAD_VALUE);
		CHK(nodeInfo.InitCheck() == B_BAD_VALUE);
	}
	// * valid node => InitCheck() == B_OK
	// * reinitialize invalid/valid => InitCheck() == B_BAD_VALUE/B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.InitCheck() == B_OK);
		// reinit with NULL node
		CHK(nodeInfo.SetTo(NULL) == B_BAD_VALUE);
		CHK(nodeInfo.InitCheck() == B_BAD_VALUE);
		// reinit with valid node
		BNode node2(testFile2);
		CHK(nodeInfo.SetTo(&node2) == B_OK);
		CHK(nodeInfo.InitCheck() == B_OK);
	}
}

// CheckAttr
static
void
CheckAttr(BNode &node, const char *name, type_code type, const void *data,
		  int32 dataSize)
{
	attr_info info;
	CHK(node.GetAttrInfo(name, &info) == B_OK);
	CHK(info.type == type);
	CHK(info.size == dataSize);
	char *buffer = new char[dataSize];
	AutoDeleter<char> deleter(buffer, true);
	CHK(node.ReadAttr(name, type, 0, buffer, dataSize) == dataSize);
	CHK(memcmp(buffer, data, dataSize) == 0);
}

// CheckNoAttr
static
void
CheckNoAttr(BNode &node, const char *name)
{
	attr_info info;
	CHK(node.GetAttrInfo(name, &info) == B_ENTRY_NOT_FOUND);
}

// CheckStringAttr
/*static
void
CheckStringAttr(BNode &node, const char *name, const char *data)
{
	CheckAttr(node, name, B_STRING_TYPE, data, strlen(data) + 1);
}*/

// CheckTypeAttr
static
void
CheckTypeAttr(BNode &node, const char *data)
{
	CheckAttr(node, kTypeAttribute, B_MIME_STRING_TYPE, data,
			  strlen(data) + 1);
}

// CheckIconAttr
static
void
CheckTypeAttr(BNode &node, BBitmap *data, icon_size iconSize)
{
	const char *attribute = (iconSize == B_MINI_ICON ? kMiniIconAttribute
													 : kLargeIconAttribute);
	CheckAttr(node, attribute, B_MIME_STRING_TYPE, data->Bits(),
			  data->BitsLength());
}

// TypeTest
void
NodeInfoTest::TypeTest()
{
	// status_t GetType(char *type) const
	// * NULL type => B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.GetType(NULL) == B_BAD_ADDRESS);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		char type[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetType(type) == B_NO_INIT);
	}
	// * has no type => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		char type[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetType(type) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// set
		CHK(nodeInfo.SetType(testType1) == B_OK);
		// get
		char type[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetType(type) == B_OK);
		CHK(strcmp(testType1, type) == 0);
		CheckTypeAttr(node, testType1);
		// reset
		CHK(nodeInfo.SetType(testType2) == B_OK);
		// get
		CHK(nodeInfo.GetType(type) == B_OK);
		CHK(strcmp(testType2, type) == 0);
		CheckTypeAttr(node, testType2);
	}

	// status_t SetType(const char *type)
	// * NULL type => B_OK, unsets the type
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.SetType(NULL) == B_OK);
		// get
		char type[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetType(type) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(node, kTypeAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetType(testType1) == B_NO_INIT);
	}
	// * invalid MIME type => B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.SetType(invalidTestType) == B_OK);
		// get
		char type[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetType(type) == B_OK);
		CHK(strcmp(invalidTestType, type) == 0);
		CheckTypeAttr(node, invalidTestType);
	}
	// * type string too long => B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.SetType(tooLongTestType) == B_OK);
		// get
		char type[1024];
		CHK(nodeInfo.GetType(type) == B_OK);
// R5: Returns a string one character shorter than the original string
#ifndef TEST_R5
		CHK(strcmp(tooLongTestType, type) == 0);
#endif
		CheckTypeAttr(node, tooLongTestType);
	}
}

// IconTest
void
NodeInfoTest::IconTest()
{
	// status_t GetIcon(BBitmap *icon, icon_size k) const
	// * NULL icon => B_BAD_VALUE
	// * unknown icon size => B_BAD_VALUE
	// * icon dimensions != icon size => B_BAD_VALUE
	// * has no icon => != B_OK
	// * set, get, reset, get

	// status_t SetIcon(const BBitmap *icon, icon_size k)
	// * NULL icon => unsets icon, B_OK
	// * unknown icon size => B_BAD_VALUE
	// * icon dimensions != icon size => B_BAD_VALUE
}

// PreferredAppTest
void
NodeInfoTest::PreferredAppTest()
{
	// status_t GetPreferredApp(char *signature, app_verb verb = B_OPEN) const
	// * NULL signature => B_BAD_VALUE
	// * verb != B_OK => B_BAD_VALUE
	// * has no preferred app => != B_OK
	// * set, get, reset, get
	// status_t SetPreferredApp(const char *signature, app_verb verb = B_OPEN)
	// * NULL signature => B_BAD_VALUE
	// * verb != B_OK => B_BAD_VALUE
}

// AppHintTest
void
NodeInfoTest::AppHintTest()
{
	// status_t GetAppHint(entry_ref *ref) const
	// * NULL ref => B_BAD_VALUE
	// * invalid/abstract ref => != B_OK
	// * has no app hint => B_ENTRY_NOT_FOUND
	// * set, get, reset, get
	// status_t SetAppHint(const entry_ref *ref)
	// * NULL ref => B_BAD_VALUE
	// * invalid/abstract ref => != B_OK
}

// TrackerIconTest
void
NodeInfoTest::TrackerIconTest()
{
	// status_t GetTrackerIcon(BBitmap *icon, icon_size k = B_LARGE_ICON) const
	// * NULL icon => B_BAD_VALUE
	// * unknown icon size => B_BAD_VALUE
	// * icon dimensions != icon size => B_BAD_VALUE
	// * has no icon, but preferred app with icon for type => B_OK
	// * no icon, no preferred app/preferred app without icon for type,
	//   but icon for type in MIME data base => B_OK
	// * no icon, no preferred app/preferred app without icon for type,
	//   no icon for type in MIME data base, preferred app for type in MIME
	//   database and app has icon for type => B_OK
	// * no icon, no preferred app/preferred app without icon for type,
	//   no icon for type in MIME data base, no preferred app for type in MIME
	//   database/app without icon for type => != B_OK
	// static status_t GetTrackerIcon(const entry_ref *ref, BBitmap *icon,
	//								  icon_size k = B_LARGE_ICON)
	// * NULL ref => B_BAD_VALUE
	// otherwise same tests
}

