// AppFileInfoTest.cpp

#include <stdio.h>
#include <string>
#include <unistd.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <Path.h>
#include <Resources.h>
#include <TypeConstants.h>

#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "AppFileInfoTest.h"
#include "../app/bmessenger/Helpers.h"

// test dirs/files/types
static const char *testDir			= "/tmp/testDir";
static const char *testFile1		= "/tmp/testDir/file1";
static const char *testFile2		= "/tmp/testDir/file2";
static const char *testFile3		= "/tmp/testDir/file3";
static const char *testFile4		= "/tmp/testDir/file4";
static const char *abstractTestEntry = "/tmp/testDir/abstract-entry";
static const char *testType1
	= "application/x-vnd.obos.app-file-info-test1";
static const char *testType2
	= "application/x-vnd.obos.app-file-info-test2";
static const char *testType3
	= "application/x-vnd.obos.app-file-info-test3";
static const char *invalidTestType	= "invalid-mime-type";
static const char *tooLongTestType	=
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
;
static const char *testAppSignature1
	= "application/x-vnd.obos.app-file-info-test-app1";
static const char *testAppSignature2
	= "application/x-vnd.obos.app-file-info-test-app2";


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
AppFileInfoTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<AppFileInfoTest> TC;
		
	suite->addTest( new TC("BAppFileInfo::Init Test1",
						   &AppFileInfoTest::InitTest1) );
	suite->addTest( new TC("BAppFileInfo::Init Test2",
						   &AppFileInfoTest::InitTest2) );
	suite->addTest( new TC("BAppFileInfo::Type Test",
						   &AppFileInfoTest::TypeTest) );
	suite->addTest( new TC("BAppFileInfo::Signature Test",
						   &AppFileInfoTest::SignatureTest) );
	suite->addTest( new TC("BAppFileInfo::App Flags Test",
						   &AppFileInfoTest::AppFlagsTest) );
	suite->addTest( new TC("BAppFileInfo::Supported Types Test",
						   &AppFileInfoTest::SupportedTypesTest) );
	suite->addTest( new TC("BAppFileInfo::Icon Test",
						   &AppFileInfoTest::IconTest) );
	suite->addTest( new TC("BAppFileInfo::Version Info Test",
						   &AppFileInfoTest::VersionInfoTest) );
	suite->addTest( new TC("BAppFileInfo::Icon For Type Test",
						   &AppFileInfoTest::IconForTypeTest) );
	suite->addTest( new TC("BAppFileInfo::Info Location Test",
						   &AppFileInfoTest::InfoLocationTest) );

	return suite;
}		

// setUp
void
AppFileInfoTest::setUp()
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
	fApplication
		= new BApplication("application/x-vnd.obos.app-file-info-test");
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
AppFileInfoTest::tearDown()
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

/*
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
*/

// CheckNoAttr
static
void
CheckNoAttr(BNode &node, const char *name)
{
	attr_info info;
	CHK(node.GetAttrInfo(name, &info) == B_ENTRY_NOT_FOUND);
}

// InitTest1
void
AppFileInfoTest::InitTest1()
{
	// BAppFileInfo()
	// * InitCheck() == B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.InitCheck() == B_NO_INIT);
		CHK(appFileInfo.IsUsingAttributes() == false);
		CHK(appFileInfo.IsUsingResources() == false);
	}

	// BAppFileInfo(BFile *file)
	// * NULL file => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BAppFileInfo appFileInfo(NULL);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == false);
		CHK(appFileInfo.IsUsingResources() == false);
	}
	// * invalid file => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BFile file;
		BAppFileInfo appFileInfo(&file);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == false);
		CHK(appFileInfo.IsUsingResources() == false);
	}
	// * valid file => InitCheck() == B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo(&file);
		CHK(appFileInfo.InitCheck() == B_OK);
		CHK(appFileInfo.IsUsingAttributes() == true);
		CHK(appFileInfo.IsUsingResources() == true);
	}
}

// InitTest2
void
AppFileInfoTest::InitTest2()
{
	// status_t SetTo(BFile *file)
	// * NULL file => InitCheck() == B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(NULL) == B_BAD_VALUE);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == false);
		CHK(appFileInfo.IsUsingResources() == false);
	}
	// * invalid file => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BFile file;
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_BAD_VALUE);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == false);
		CHK(appFileInfo.IsUsingResources() == false);
	}
	// * valid file => InitCheck() == B_OK
	// * reinitialize invalid/valid => InitCheck() == B_BAD_VALUE/B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.InitCheck() == B_OK);
		CHK(appFileInfo.IsUsingAttributes() == true);
		CHK(appFileInfo.IsUsingResources() == true);
		off_t size;
		CHK(file.GetSize(&size) == B_OK);
		CHK(size == 0);
		CheckNoAttr(file, kTypeAttribute);
		CheckNoAttr(file, kMiniIconAttribute);
		CheckNoAttr(file, kLargeIconAttribute);
		CheckNoAttr(file, kPreferredAppAttribute);
		CheckNoAttr(file, kAppHintAttribute);
		// reinit with NULL file
		CHK(appFileInfo.SetTo(NULL) == B_BAD_VALUE);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == true);
		CHK(appFileInfo.IsUsingResources() == true);
		CHK(file.GetSize(&size) == B_OK);
		CHK(size == 0);
		CheckNoAttr(file, kTypeAttribute);
		CheckNoAttr(file, kMiniIconAttribute);
		CheckNoAttr(file, kLargeIconAttribute);
		CheckNoAttr(file, kPreferredAppAttribute);
		CheckNoAttr(file, kAppHintAttribute);
		// reinit with valid file
		BFile file2(testFile2, B_READ_WRITE);
		CHK(appFileInfo.SetTo(&file2) == B_OK);
		CHK(appFileInfo.InitCheck() == B_OK);
		CHK(appFileInfo.IsUsingAttributes() == true);
		CHK(appFileInfo.IsUsingResources() == true);
	}
}

// TypeTest
void
AppFileInfoTest::TypeTest()
{
//	virtual status_t GetType(char *type) const;
//	virtual status_t SetType(const char *type);
}

// SignatureTest
void
AppFileInfoTest::SignatureTest()
{
//	status_t GetSignature(char *signature) const;
//	status_t SetSignature(const char *signature);
}

// AppFlagsTest
void
AppFileInfoTest::AppFlagsTest()
{
//	status_t GetAppFlags(uint32 *flags) const;
//	status_t SetAppFlags(uint32 flags);
}

// SupportedTypesTest
void
AppFileInfoTest::SupportedTypesTest()
{
//	status_t GetSupportedTypes(BMessage *types) const;
//	status_t SetSupportedTypes(const BMessage *types, bool syncAll);
//	status_t SetSupportedTypes(const BMessage *types);
//	bool IsSupportedType(const char *type) const;
//	bool Supports(BMimeType *type) const;
}

// IconTest
void
AppFileInfoTest::IconTest()
{
//	virtual status_t GetIcon(BBitmap *icon, icon_size which) const;
//	virtual status_t SetIcon(const BBitmap *icon, icon_size which);
}

// VersionInfoTest
void
AppFileInfoTest::VersionInfoTest()
{
//	status_t GetVersionInfo(version_info *info, version_kind kind) const;
//	status_t SetVersionInfo(const version_info *info, version_kind kind);
}

// IconForTypeTest
void
AppFileInfoTest::IconForTypeTest()
{
//	status_t GetIconForType(const char *type, BBitmap *icon,
//							icon_size which) const;
//	status_t SetIconForType(const char *type, const BBitmap *icon,
//							icon_size which);
}


// InfoLocationTester
template<typename Value, typename Setter, typename Getter, typename Checker>
static
void
InfoLocationTester(const Value &testValue1, const Value &testValue2,
				   const Value &testValue3)
{
	BFile file(testFile1, B_READ_WRITE);
	BAppFileInfo appFileInfo;
	CHK(appFileInfo.SetTo(&file) == B_OK);
	CHK(appFileInfo.InitCheck() == B_OK);
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == true);
	// set value
	CHK(Setter::Set(appFileInfo, testValue1) == B_OK);
	// force resources synchronization
	CHK(appFileInfo.SetTo(&file) == B_OK);
	// get value
	Value value;
	CHK(Getter::Get(appFileInfo, value) == B_OK);
	CHK(value == testValue1);
	// check attribute/resource
	Checker::CheckAttribute(file, testValue1);
	Checker::CheckResource(file, testValue1);
	// set on B_USE_ATTRIBUTES
	appFileInfo.SetInfoLocation(B_USE_ATTRIBUTES);
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == false);
	// set value
	CHK(Setter::Set(appFileInfo, testValue2) == B_OK);
	// force resources synchronization
	CHK(appFileInfo.SetTo(&file) == B_OK);
	appFileInfo.SetInfoLocation(B_USE_ATTRIBUTES);
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == false);
	// get value
	Value value1;
	CHK(Getter::Get(appFileInfo, value1) == B_OK);
	CHK(value1 == testValue2);
	// check attribute/resource -- resource should be old
	Checker::CheckAttribute(file, testValue2);
	Checker::CheckResource(file, testValue1);
	// set on B_USE_BOTH_LOCATIONS
	appFileInfo.SetInfoLocation(B_USE_BOTH_LOCATIONS);
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == true);
	// get value -- the attribute is preferred
	Value value2;
	CHK(Getter::Get(appFileInfo, value2) == B_OK);
	CHK(value2 == testValue2);
	// set on B_USE_RESOURCES
	appFileInfo.SetInfoLocation(B_USE_RESOURCES);
	CHK(appFileInfo.IsUsingAttributes() == false);
	CHK(appFileInfo.IsUsingResources() == true);
	// set value
	CHK(Setter::Set(appFileInfo, testValue3) == B_OK);
	// force resources synchronization
	CHK(appFileInfo.SetTo(&file) == B_OK);
	appFileInfo.SetInfoLocation(B_USE_RESOURCES);
	CHK(appFileInfo.IsUsingAttributes() == false);
	CHK(appFileInfo.IsUsingResources() == true);
	// get value
	Value value3;
	CHK(Getter::Get(appFileInfo, value3) == B_OK);
	CHK(value3 == testValue3);
	// check attribute/resource -- attribute should be old
	Checker::CheckAttribute(file, testValue2);
	Checker::CheckResource(file, testValue3);
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

// CheckResource
static
void
CheckResource(BFile &file, const char *name, int32 id, type_code type,
			  const void *data, size_t dataSize)
{
	BResources resources;
	CHK(resources.SetTo(&file) == B_OK);
	int32 idFound;
	size_t size;
	CHK(resources.GetResourceInfo(type, name, &idFound, &size) == true);
	CHK(idFound == id);
	CHK(size == dataSize);
	const void *resourceData = resources.LoadResource(type, name, &size);
	CHK(resourceData != NULL);
	CHK(size == dataSize);
	CHK(memcmp(resourceData, data, dataSize) == 0);
}

// TypeValue
struct TypeValue
{
	TypeValue() : type() {}
	TypeValue(const char *type) : type(type) {}

	bool operator==(const TypeValue &value)
	{
		return (type == value.type);
	}

	string type;
};

// TypeSetter
struct TypeSetter
{
	static status_t Set(BAppFileInfo &info, const TypeValue &value)
	{
		return info.SetType(value.type.c_str());
	}
};

// TypeGetter
struct TypeGetter
{
	static status_t Get(BAppFileInfo &info, TypeValue &value)
	{
		char buffer[B_MIME_TYPE_LENGTH];
		status_t error = info.GetType(buffer);
		if (error == B_OK)
			value.type = buffer;
		return error;
	}
};

const int32 kTypeResourceID = 2;

// TypeChecker
struct TypeChecker
{
	static void CheckAttribute(BFile &file, const TypeValue &value)
	{
		CheckAttr(file, kTypeAttribute, B_MIME_STRING_TYPE, value.type.c_str(),
				  value.type.length() + 1);
	}

	static void CheckResource(BFile &file, const TypeValue &value)
	{
		::CheckResource(file, kTypeAttribute, kTypeResourceID,
						B_MIME_STRING_TYPE, value.type.c_str(),
						value.type.length() + 1);
	}
};


// InfoLocationTest
void
AppFileInfoTest::InfoLocationTest()
{
//	void SetInfoLocation(info_location location);
//	bool IsUsingAttributes() const;
//	bool IsUsingResources() const;

	InfoLocationTester<TypeValue, TypeSetter, TypeGetter, TypeChecker>(
		TypeValue(testType1), TypeValue(testType2), TypeValue(testType3));
}

