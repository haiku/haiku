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
static const char *testAppSignature1
	= "application/x-vnd.obos.node-info-test-app1";
static const char *testAppSignature2
	= "application/x-vnd.obos.node-info-test-app2";


// attributes
static const char *kTypeAttribute			= "BEOS:TYPE";
static const char *kMiniIconAttribute		= "BEOS:M:STD_ICON";
static const char *kLargeIconAttribute		= "BEOS:L:STD_ICON";
static const char *kPreferredAppAttribute	= "BEOS:PREF_APP";
//static const char *kAppHintAttribute		= "BEOS:PPATH";

enum {
	MINI_ICON_TYPE	= 'MICN',
	LARGE_ICON_TYPE	= 'ICON',
};

// create_test_icon
static
BBitmap *
create_test_icon(icon_size size, int fill)
{
	BBitmap *icon = NULL;
	// create
	switch (size) {
		case B_MINI_ICON:
			icon = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			break;
		case B_LARGE_ICON:
			icon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			break;
	}
	// fill
	if (icon)
		memset(icon->Bits(), fill, icon->BitsLength());
	return icon;
}

// icon_equal
static
bool
icon_equal(const BBitmap *icon1, const BBitmap *icon2)
{
	return (icon1->Bounds() == icon2->Bounds()
			&& icon1->BitsLength() == icon2->BitsLength()
			&& memcmp(icon1->Bits(), icon2->Bits(), icon1->BitsLength()) == 0);
}


// Suite
CppUnit::Test*
NodeInfoTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<NodeInfoTest> TC;
		
//	suite->addTest( new TC("BNodeInfo::Init Test1", &NodeInfoTest::InitTest1) );
//	suite->addTest( new TC("BNodeInfo::Init Test2", &NodeInfoTest::InitTest2) );
//	suite->addTest( new TC("BNodeInfo::Type Test", &NodeInfoTest::TypeTest) );
//	suite->addTest( new TC("BNodeInfo::Icon Test", &NodeInfoTest::IconTest) );
	suite->addTest( new TC("BNodeInfo::Preferred App Test",
						   &NodeInfoTest::PreferredAppTest) );
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
	// create test dir and files
	execCommand(
		string("mkdir ") + testDir
		+ "; touch " + testFile1
		+ "; touch " + testFile2
	);
	// create app
	fApplication = new BApplication("application/x-vnd.obos.node-info-test");
	// create icons
	fIconM1 = create_test_icon(B_MINI_ICON, 1);
	fIconM2 = create_test_icon(B_MINI_ICON, 2);
	fIconL1 = create_test_icon(B_LARGE_ICON, 1);
	fIconL2 = create_test_icon(B_LARGE_ICON, 2);
}
	
// tearDown
void
NodeInfoTest::tearDown()
{
	// delete the icons
	delete fIconM1;
	delete fIconM2;
	delete fIconL1;
	delete fIconL2;
	fIconM1 = fIconM2 = fIconL1 = fIconL2 = NULL;
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
CheckIconAttr(BNode &node, BBitmap *data)
{
	const char *attribute = NULL;
	uint32 type = 0;
	switch (data->Bounds().IntegerWidth()) {
		case 15:
			attribute = kMiniIconAttribute;
			type = MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconAttribute;
			type = LARGE_ICON_TYPE;
			break;
		default:
			CHK(false);
			break;
	}
	CheckAttr(node, attribute, type, data->Bits(), data->BitsLength());
}

// CheckPreferredAppAttr
static
void
CheckPreferredAppAttr(BNode &node, const char *data)
{
	CheckAttr(node, kPreferredAppAttribute, B_MIME_STRING_TYPE, data,
			  strlen(data) + 1);
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
// R5: Crashes when passing a NULL icon.
#ifndef TEST_R5
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.GetIcon(NULL, B_MINI_ICON) == B_BAD_VALUE);
		CHK(nodeInfo.GetIcon(NULL, B_LARGE_ICON) == B_BAD_VALUE);
	}
#endif
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon, B_MINI_ICON) == B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon2, B_LARGE_ICON) == B_NO_INIT);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon, B_MINI_ICON) == B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon2, B_LARGE_ICON) == B_BAD_VALUE);
	}
	// * has no icon => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon, B_MINI_ICON) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// mini
		// set
		CHK(nodeInfo.SetIcon(fIconM1, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM1, &icon));
		CheckIconAttr(node, fIconM1);
		// reset
		CHK(nodeInfo.SetIcon(fIconM2, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon2, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM2, &icon2));
		CheckIconAttr(node, fIconM2);
		// large
		// set
		CHK(nodeInfo.SetIcon(fIconL1, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon3, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL1, &icon3));
		CheckIconAttr(node, fIconL1);
		// reset
		CHK(nodeInfo.SetIcon(fIconL2, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon4, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL2, &icon4));
		CheckIconAttr(node, fIconL2);
	}
	// * bitmap color_space != B_CMAP8 => B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// mini
		BBitmap icon(BRect(0, 0, 15, 15), B_RGB32);
		CHK(nodeInfo.GetIcon(&icon, B_MINI_ICON) == B_OK);
		BBitmap icon2(BRect(0, 0, 15, 15), B_RGB32);
		icon2.SetBits(fIconM2->Bits(), fIconM2->BitsLength(), 0, B_CMAP8);
		CHK(icon_equal(&icon, &icon2));
		// large
// R5: Crashes for some weird reason in GetIcon().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CHK(nodeInfo.GetIcon(&icon3, B_LARGE_ICON) == B_OK);
		BBitmap icon4(BRect(0, 0, 31, 31), B_RGB32);
		icon4.SetBits(fIconL2->Bits(), fIconL2->BitsLength(), 0, B_CMAP8);
		CHK(icon_equal(&icon3, &icon4));
#endif
	}

	// status_t SetIcon(const BBitmap *icon, icon_size k)
	// * NULL icon => unsets icon, B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// mini
		// set
		CHK(nodeInfo.SetIcon(NULL, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon, B_MINI_ICON) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(node, kMiniIconAttribute);
		// large
		// set
		CHK(nodeInfo.SetIcon(NULL, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetIcon(&icon2, B_LARGE_ICON) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(node, kLargeIconAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.SetIcon(&icon, B_MINI_ICON) == B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.SetIcon(&icon2, B_LARGE_ICON) == B_NO_INIT);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.SetIcon(&icon, B_MINI_ICON) == B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.SetIcon(&icon2, B_LARGE_ICON) == B_BAD_VALUE);
	}
}

// PreferredAppTest
void
NodeInfoTest::PreferredAppTest()
{
	// status_t GetPreferredApp(char *signature, app_verb verb = B_OPEN) const
	// * NULL signature => B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.GetPreferredApp(NULL) == B_BAD_ADDRESS);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		char signature[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetPreferredApp(signature) == B_NO_INIT);
	}
	// * has no preferred app => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		char signature[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetPreferredApp(signature) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// set
		CHK(nodeInfo.SetPreferredApp(testAppSignature1) == B_OK);
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetPreferredApp(signature) == B_OK);
		CHK(strcmp(testAppSignature1, signature) == 0);
		CheckPreferredAppAttr(node, testAppSignature1);
		// reset
		CHK(nodeInfo.SetPreferredApp(testAppSignature2) == B_OK);
		// get
		CHK(nodeInfo.GetPreferredApp(signature) == B_OK);
		CHK(strcmp(testAppSignature2, signature) == 0);
		CheckPreferredAppAttr(node, testAppSignature2);
	}

	// status_t SetPreferredApp(const char *signature, app_verb verb = B_OPEN)
	// * NULL signature => unsets the preferred app
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.SetPreferredApp(NULL) == B_OK);
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetPreferredApp(signature) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(node, kPreferredAppAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetPreferredApp(testAppSignature1) == B_NO_INIT);
	}
	// * invalid MIME type => B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.SetPreferredApp(invalidTestType) == B_OK);
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CHK(nodeInfo.GetPreferredApp(signature) == B_OK);
		CHK(strcmp(invalidTestType, signature) == 0);
		CheckPreferredAppAttr(node, invalidTestType);
	}
	// * signature string too long => B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// unset
		CHK(nodeInfo.SetPreferredApp(NULL) == B_OK);
		// try to set
		CHK(nodeInfo.SetPreferredApp(tooLongTestType) == B_BAD_VALUE);
		// get
		char signature[1024];
		CHK(nodeInfo.GetPreferredApp(signature) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(node, kPreferredAppAttribute);
	}
}

// AppHintTest
void
NodeInfoTest::AppHintTest()
{
	// status_t GetAppHint(entry_ref *ref) const
	// * NULL ref => B_BAD_VALUE
	// * uninitialized => B_NO_INIT
	// * invalid/abstract ref => != B_OK
	// * has no app hint => B_ENTRY_NOT_FOUND
	// * set, get, reset, get
	// status_t SetAppHint(const entry_ref *ref)
	// * NULL ref => B_BAD_VALUE
	// * uninitialized => B_NO_INIT
	// * invalid/abstract ref => != B_OK
}

// TrackerIconTest
void
NodeInfoTest::TrackerIconTest()
{
	// status_t GetTrackerIcon(BBitmap *icon, icon_size k = B_LARGE_ICON) const
	// * NULL icon => B_BAD_VALUE
	// * uninitialized => B_NO_INIT
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

