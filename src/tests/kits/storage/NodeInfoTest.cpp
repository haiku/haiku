// NodeInfoTest.cpp

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>
using std::string;

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TypeConstants.h>
#include <MimeTypes.h>

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
static const char *testFile3		= "/tmp/testDir/file3";
static const char *testFile4		= "/tmp/testDir/file4";
static const char *abstractTestEntry = "/tmp/testDir/abstract-entry";
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
static const char *kAppHintAttribute		= "BEOS:PPATH";

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
		
	suite->addTest( new TC("BNodeInfo::Init Test1", &NodeInfoTest::InitTest1) );
	suite->addTest( new TC("BNodeInfo::Init Test2", &NodeInfoTest::InitTest2) );
	suite->addTest( new TC("BNodeInfo::Type Test", &NodeInfoTest::TypeTest) );
	suite->addTest( new TC("BNodeInfo::Icon Test", &NodeInfoTest::IconTest) );
	suite->addTest( new TC("BNodeInfo::Preferred App Test",
						   &NodeInfoTest::PreferredAppTest) );
	suite->addTest( new TC("BNodeInfo::App Hint Test",
						   &NodeInfoTest::AppHintTest) );
	suite->addTest( new TC("BNodeInfo::Tracker Icon Test",
						   &NodeInfoTest::TrackerIconTest) );

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
			   + " " + testFile2
			   + " " + testFile3
			   + " " + testFile4
	);
	// create app
	fApplication = new BApplication("application/x-vnd.obos.node-info-test");
	// create icons
	fIconM1 = create_test_icon(B_MINI_ICON, 1);
	fIconM2 = create_test_icon(B_MINI_ICON, 2);
	fIconM3 = create_test_icon(B_MINI_ICON, 3);
	fIconM4 = create_test_icon(B_MINI_ICON, 4);
	fIconL1 = create_test_icon(B_LARGE_ICON, 1);
	fIconL2 = create_test_icon(B_LARGE_ICON, 2);
	fIconL3 = create_test_icon(B_LARGE_ICON, 3);
	fIconL4 = create_test_icon(B_LARGE_ICON, 4);
}
	
// tearDown
void
NodeInfoTest::tearDown()
{
	// delete the icons
	delete fIconM1;
	delete fIconM2;
	delete fIconM3;
	delete fIconM4;
	delete fIconL1;
	delete fIconL2;
	delete fIconL3;
	delete fIconL4;
	fIconM1 = fIconM2 = fIconL1 = fIconL2 = NULL;
	// delete the application
	delete fApplication;
	fApplication = NULL;
	// remove the types we've added
	const char * const testTypes[] = {
		testType1, testType2, testAppSignature1, testAppSignature2
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

// CheckAppHintAttr
static
void
CheckAppHintAttr(BNode &node, const entry_ref *ref)
{
	BPath path;
	CHK(path.SetTo(ref) == B_OK);
	const char *data = path.Path();
// R5: Attribute is of type B_MIME_STRING_TYPE though it contains a path name!
	CheckAttr(node, kAppHintAttribute, B_MIME_STRING_TYPE, data,
			  strlen(data) + 1);
}

// TypeTest
void
NodeInfoTest::TypeTest()
{
	// status_t GetType(char *type) const
	// * NULL type => B_BAD_ADDRESS/B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(equals(nodeInfo.GetType(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
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
	// * type string too long => B_OK/B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// remove attr first
		CHK(nodeInfo.SetType(NULL) == B_OK);
// R5: Doesn't complain when setting a too long string.
// Haiku: Handles this as an error case.
#ifdef TEST_R5
		CHK(nodeInfo.SetType(tooLongTestType) == B_OK);
		// get
		char type[1024];
		CHK(nodeInfo.GetType(type) == B_OK);
// R5: Returns a string one character shorter than the original string
//		CHK(strcmp(tooLongTestType, type) == 0);
		CheckTypeAttr(node, tooLongTestType);
#else
		CHK(nodeInfo.SetType(tooLongTestType) == B_BAD_VALUE);
		CheckNoAttr(node, kTypeAttribute);
#endif
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
		// SetBits() can be used, since there's no row padding for 16x16.
		icon2.SetBits(fIconM2->Bits(), fIconM2->BitsLength(), 0, B_CMAP8);
		CHK(icon_equal(&icon, &icon2));
		// large
// R5: Crashes for some weird reason in GetIcon().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CHK(nodeInfo.GetIcon(&icon3, B_LARGE_ICON) == B_OK);
		BBitmap icon4(BRect(0, 0, 31, 31), B_RGB32);
		// SetBits() can be used, since there's no row padding for 32x32.
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
	// * NULL signature => B_BAD_ADDRESS/B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(equals(nodeInfo.GetPreferredApp(NULL), B_BAD_ADDRESS,
				   B_BAD_VALUE));
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
	// init test refs
	entry_ref testRef1, testRef2, abstractRef;
	CHK(get_ref_for_path(testFile3, &testRef1) == B_OK);
	CHK(get_ref_for_path(testFile4, &testRef2) == B_OK);
	CHK(get_ref_for_path(abstractTestEntry, &abstractRef) == B_OK);

	// status_t GetAppHint(entry_ref *ref) const
	// * NULL ref => B_BAD_VALUE
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.GetAppHint(NULL) == B_BAD_VALUE);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		entry_ref ref;
		CHK(nodeInfo.GetAppHint(&ref) == B_NO_INIT);
	}
	// * has no app hint => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		entry_ref ref;
		CHK(nodeInfo.GetAppHint(&ref) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// set
		CHK(nodeInfo.SetAppHint(&testRef1) == B_OK);
		// get
		entry_ref ref;
		CHK(nodeInfo.GetAppHint(&ref) == B_OK);
		CHK(ref == testRef1);
		CheckAppHintAttr(node, &testRef1);
		// reset
		CHK(nodeInfo.SetAppHint(&testRef2) == B_OK);
		// get
		CHK(nodeInfo.GetAppHint(&ref) == B_OK);
		CHK(ref == testRef2);
		CheckAppHintAttr(node, &testRef2);
	}

	// status_t SetAppHint(const entry_ref *ref)
	// * NULL ref => B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.SetAppHint(NULL) == B_OK);
		// get
		entry_ref ref;
		CHK(nodeInfo.GetAppHint(&ref) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(node, kAppHintAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetAppHint(&testRef1) == B_NO_INIT);
	}
	// * invalid/abstract ref => != B_OK
	NextSubTest();
	{
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		// invalid ref
		entry_ref invalidRef;
		CHK(nodeInfo.SetAppHint(&invalidRef) != B_OK);
		// abstract ref
		CHK(nodeInfo.SetAppHint(&abstractRef) == B_OK);
		// get
		entry_ref ref;
		CHK(nodeInfo.GetAppHint(&ref) == B_OK);
		CHK(ref == abstractRef);
		CheckAppHintAttr(node, &abstractRef);
	}
}

// TestTrackerIcon
static
void
TestTrackerIcon(BNodeInfo &nodeInfo, entry_ref *ref, icon_size size,
				BBitmap *expectedIcon)
{
	// mini
	if (size == B_MINI_ICON) {
		// non-static
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetTrackerIcon(&icon, B_MINI_ICON) == B_OK);
		CHK(icon_equal(expectedIcon, &icon));
		// static
		BBitmap icon1(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(BNodeInfo::GetTrackerIcon(ref, &icon1, B_MINI_ICON) == B_OK);
		CHK(icon_equal(expectedIcon, &icon1));
	} else {
		// large
		// non-static
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetTrackerIcon(&icon2, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(expectedIcon, &icon2));
		// static
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(BNodeInfo::GetTrackerIcon(ref, &icon3, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(expectedIcon, &icon3));
	}
}


static void
TestTrackerIcon(const char *path, const char *type)
{
	// preparation for next tests: get icons for specified type
	BBitmap miniIcon(BRect(0, 0, 15, 15), B_CMAP8);
	BBitmap largeIcon(BRect(0, 0, 31, 31), B_CMAP8);
	BMimeType mimeType(type);
	CHK(mimeType.GetIcon(&miniIcon, B_MINI_ICON) == B_OK);
	CHK(mimeType.GetIcon(&largeIcon, B_LARGE_ICON) == B_OK);

	BNode node(path);
	CHK(node.InitCheck() == B_OK);
	BEntry entry(path);
	CHK(entry.InitCheck() == B_OK);
	entry_ref ref;
	CHK(entry.GetRef(&ref) == B_OK);
	BNodeInfo info(&node);
	CHK(info.InitCheck() == B_OK);

	// test GetTrackerIcon()
	TestTrackerIcon(info, &ref, B_MINI_ICON, &miniIcon);
	TestTrackerIcon(info, &ref, B_LARGE_ICON, &largeIcon);
}


// TrackerIconTest
void
NodeInfoTest::TrackerIconTest()
{
	entry_ref testRef1;
	CHK(get_ref_for_path(testFile1, &testRef1) == B_OK);
	BBitmap octetMIcon(BRect(0, 0, 15, 15), B_CMAP8);
	BBitmap octetLIcon(BRect(0, 0, 31, 31), B_CMAP8);
	BMimeType octetType(B_FILE_MIME_TYPE);
	CHK(octetType.GetIcon(&octetMIcon, B_MINI_ICON) == B_OK);
	CHK(octetType.GetIcon(&octetLIcon, B_LARGE_ICON) == B_OK);

	// static status_t GetTrackerIcon(const entry_ref *ref, BBitmap *icon,
	//								  icon_size k = B_LARGE_ICON)
	// * NULL ref => B_BAD_VALUE
	NextSubTest();
	{
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(BNodeInfo::GetTrackerIcon(NULL, &icon, B_MINI_ICON)
			== B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(BNodeInfo::GetTrackerIcon(NULL, &icon2, B_LARGE_ICON)
			== B_BAD_VALUE);
	}

	// status_t GetTrackerIcon(BBitmap *icon, icon_size k = B_LARGE_ICON) const
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BNodeInfo nodeInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetTrackerIcon(&icon, B_MINI_ICON) == B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetTrackerIcon(&icon2, B_LARGE_ICON) == B_NO_INIT);
	}

	// status_t GetTrackerIcon(BBitmap *icon, icon_size k = B_LARGE_ICON) const
	// static status_t GetTrackerIcon(const entry_ref *ref, BBitmap *icon,
	//								  icon_size k = B_LARGE_ICON)
	// * NULL icon => B_BAD_VALUE
	NextSubTest();
	{
		// non-static
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		CHK(nodeInfo.GetTrackerIcon(NULL, B_MINI_ICON) == B_BAD_VALUE);
		CHK(nodeInfo.GetTrackerIcon(NULL, B_LARGE_ICON) == B_BAD_VALUE);
		// static
		CHK(BNodeInfo::GetTrackerIcon(&testRef1, NULL, B_MINI_ICON)
			== B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(BNodeInfo::GetTrackerIcon(&testRef1, NULL, B_LARGE_ICON)
			== B_BAD_VALUE);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		// non-static
		BNode node(testFile1);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&node) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(nodeInfo.GetTrackerIcon(&icon, B_MINI_ICON) == B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(nodeInfo.GetTrackerIcon(&icon2, B_LARGE_ICON) == B_BAD_VALUE);
		// static
		CHK(BNodeInfo::GetTrackerIcon(&testRef1, &icon, B_MINI_ICON)
			== B_BAD_VALUE);
		CHK(BNodeInfo::GetTrackerIcon(&testRef1, &icon2, B_LARGE_ICON)
			== B_BAD_VALUE);
	}

	// initialization for further tests
	BNode node(testFile1);
	BNodeInfo nodeInfo;
	CHK(nodeInfo.SetTo(&node) == B_OK);
	// install file type
	BMimeType type(testType1);
	CHK(type.Install() == B_OK);
	// set icons for file
	CHK(nodeInfo.SetIcon(fIconM1, B_MINI_ICON) == B_OK);
	CHK(nodeInfo.SetIcon(fIconL1, B_LARGE_ICON) == B_OK);
	// install application type with icons for type, and make it the file's
	// preferred application
	BMimeType appType(testAppSignature1);
	CHK(appType.Install() == B_OK);
	CHK(appType.SetIconForType(testType1, fIconM2, B_MINI_ICON) == B_OK);
	CHK(appType.SetIconForType(testType1, fIconL2, B_LARGE_ICON) == B_OK);
	CHK(nodeInfo.SetPreferredApp(testAppSignature1) == B_OK);
	// set icons for type in MIME database
	CHK(type.SetIcon(fIconM3, B_MINI_ICON) == B_OK);
	CHK(type.SetIcon(fIconL3, B_LARGE_ICON) == B_OK);
	// install application type with icons for type, and make it the type's
	// preferred application
	BMimeType appType2(testAppSignature2);
	CHK(appType2.Install() == B_OK);
	CHK(appType2.SetIconForType(testType1, fIconM4, B_MINI_ICON) == B_OK);
	CHK(appType2.SetIconForType(testType1, fIconL4, B_LARGE_ICON) == B_OK);
	CHK(type.SetPreferredApp(testAppSignature2) == B_OK);

	// * has icon, but not type => B_OK,
	//   returns the "application/octet-stream" icon
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, &octetMIcon);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, &octetLIcon);
	}
	// set invalid file type
	CHK(nodeInfo.SetType(invalidTestType) == B_OK);
	// * has icon, but invalid type => B_OK, returns file icon
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, fIconM1);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, fIconL1);
	}
	// set file type
	CHK(nodeInfo.SetType(testType1) == B_OK);
	// * has icon => B_OK
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, fIconM1);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, fIconL1);
	}
	// unset icons
	CHK(nodeInfo.SetIcon(NULL, B_MINI_ICON) == B_OK);
	CHK(nodeInfo.SetIcon(NULL, B_LARGE_ICON) == B_OK);
	// * has no icon, but preferred app with icon for type => B_OK
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, fIconM2);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, fIconL2);
	}
	// unset icons for type for preferred app
	CHK(appType.SetIconForType(testType1, NULL, B_MINI_ICON) == B_OK);
	CHK(appType.SetIconForType(testType1, NULL, B_LARGE_ICON) == B_OK);
	// * no icon, preferred app without icon for type,
	//   but icon for type in MIME data base => B_OK
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, fIconM3);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, fIconL3);
	}
	// unset preferred app
	CHK(nodeInfo.SetPreferredApp(NULL) == B_OK);
	// * no icon, no preferred app,
	//   but icon for type in MIME data base => B_OK
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, fIconM3);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, fIconL3);
	}
	// unset icons for type
	CHK(type.SetIcon(NULL, B_MINI_ICON) == B_OK);
	CHK(type.SetIcon(NULL, B_LARGE_ICON) == B_OK);
	// * no icon, no preferred app, no icon for type in MIME data base, but
	//   preferred app for type in MIME database and app has icon for type
	//   => B_OK
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, fIconM4);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, fIconL4);
	}
	// unset icons for type for preferred app
	CHK(appType2.SetIconForType(testType1, NULL, B_MINI_ICON) == B_OK);
	CHK(appType2.SetIconForType(testType1, NULL, B_LARGE_ICON) == B_OK);
	// * no icon, no preferred app, no icon for type in MIME data base,
	//   preferred app for type in MIME database and app has no icon for type
	//   => B_OK, returns the "application/octet-stream" icon
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, &octetMIcon);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, &octetLIcon);
	}
	// unset preferred application
	CHK(type.SetPreferredApp(NULL) == B_OK);
	// * no icon, no preferred app, no icon for type in MIME data base,
	//   no preferred app for type in MIME database => B_OK,
	//   returns the "application/octet-stream" icon
	NextSubTest();
	{
		TestTrackerIcon(nodeInfo, &testRef1, B_MINI_ICON, &octetMIcon);
		TestTrackerIcon(nodeInfo, &testRef1, B_LARGE_ICON, &octetLIcon);
	}

	// Test GetTrackerIcon() for different types (without type set)
	NextSubTest();
	{
		TestTrackerIcon(testDir, B_DIRECTORY_MIME_TYPE);
		TestTrackerIcon("/", B_VOLUME_MIME_TYPE);
		TestTrackerIcon("/system", B_SYMLINK_MIME_TYPE);

		chmod(testFile4, 0755);
		TestTrackerIcon(testFile4, B_APP_MIME_TYPE);
		chmod(testFile4, 0644);
	}
}

