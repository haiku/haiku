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
#include <Roster.h>
#include <String.h>
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
//static const char *abstractTestEntry = "/tmp/testDir/abstract-entry";
static const char *testType1
	= "application/x-vnd.obos.app-file-info-test1";
static const char *testType2
	= "application/x-vnd.obos.app-file-info-test2";
static const char *testType3
	= "application/x-vnd.obos.app-file-info-test3";
//static const char *invalidTestType	= "invalid-mime-type";
//static const char *tooLongTestType	=
//"0123456789012345678901234567890123456789012345678901234567890123456789"
//"0123456789012345678901234567890123456789012345678901234567890123456789"
//"0123456789012345678901234567890123456789012345678901234567890123456789"
//"0123456789012345678901234567890123456789012345678901234567890123456789"
//;
static const char *testAppSignature1
	= "application/x-vnd.obos.app-file-info-test-app1";
static const char *testAppSignature2
	= "application/x-vnd.obos.app-file-info-test-app2";
static const char *testAppSignature3
	= "application/x-vnd.obos.app-file-info-test-app3";


// attributes
static const char *kTypeAttribute				= "BEOS:TYPE";
static const char *kSignatureAttribute			= "BEOS:APP_SIG";
static const char *kAppFlagsAttribute			= "BEOS:APP_FLAGS";
static const char *kSupportedTypesAttribute		= "BEOS:FILE_TYPES";
static const char *kMiniIconAttribute			= "BEOS:M:STD_ICON";
static const char *kLargeIconAttribute			= "BEOS:L:STD_ICON";
static const char *kVersionInfoAttribute		= "BEOS:APP_VERSION";
static const char *kMiniIconForTypeAttribute	= "BEOS:M:";
static const char *kLargeIconForTypeAttribute	= "BEOS:L:";

// resource IDs
static const int32 kTypeResourceID				= 2;
static const int32 kSignatureResourceID			= 1;
static const int32 kAppFlagsResourceID			= 1;
static const int32 kSupportedTypesResourceID	= 1;
static const int32 kMiniIconResourceID			= 101;
static const int32 kLargeIconResourceID			= 101;
static const int32 kVersionInfoResourceID		= 1;
static const int32 kMiniIconForTypeResourceID	= 0;
static const int32 kLargeIconForTypeResourceID	= 0;


enum {
	APP_FLAGS_TYPE				= 'APPF',
	MINI_ICON_TYPE				= 'MICN',
	LARGE_ICON_TYPE				= 'ICON',
	VERSION_INFO_TYPE			= 'APPV',
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

// == (version_info)
static
bool
operator==(const version_info &info1, const version_info &info2)
{
	return (info1.major == info2.major
			&& info1.middle == info2.middle
			&& info1.minor == info2.minor
			&& info1.variety == info2.variety
			&& info1.internal == info2.internal
			&& !strcmp(info1.short_info, info2.short_info)
			&& !strcmp(info1.long_info, info2.long_info));
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
		testType1, testType2, testType3,
		testAppSignature1, testAppSignature2, testAppSignature3
	};
	for (uint32 i = 0; i < sizeof(testTypes) / sizeof(const char*); i++) {
		BMimeType type(testTypes[i]);
		type.Delete();
	}

	// delete the test dir
	execCommand(string("rm -rf ") + testDir);

	BasicTest::tearDown();
}

// ReadAttr
static
char*
ReadAttr(BNode &node, const char *name, type_code type, size_t &size)
{
	attr_info info;
	CHK(node.GetAttrInfo(name, &info) == B_OK);
	CHK(info.type == type);
	char *buffer = new char[info.size];
	if (node.ReadAttr(name, type, 0, buffer, info.size) == info.size)
		size = info.size;
	else {
		delete[] buffer;
		CHK(false);
	}
	return buffer;
}

// ReadResource
static
char*
ReadResource(BFile &file, const char *name, int32 id, type_code type,
			 size_t &size)
{
	BResources resources;
	CHK(resources.SetTo(&file) == B_OK);
	int32 idFound;
	CHK(resources.GetResourceInfo(type, name, &idFound, &size) == true);
	CHK(idFound == id);
	const void *resourceData = resources.LoadResource(type, name, &size);
	CHK(resourceData != NULL);
	char *buffer = new char[size];
	memcpy(buffer, resourceData, size);
	return buffer;
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
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == true);
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
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == true);
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
	CHK(appFileInfo.IsUsingAttributes() == true);
	CHK(appFileInfo.IsUsingResources() == true);
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

typedef TypeValue SignatureValue;

// SignatureSetter
struct SignatureSetter
{
	static status_t Set(BAppFileInfo &info, const SignatureValue &value)
	{
		return info.SetSignature(value.type.c_str());
	}
};

// SignatureGetter
struct SignatureGetter
{
	static status_t Get(BAppFileInfo &info, SignatureValue &value)
	{
		char buffer[B_MIME_TYPE_LENGTH];
		status_t error = info.GetSignature(buffer);
		if (error == B_OK)
			value.type = buffer;
		return error;
	}
};

// SignatureChecker
struct SignatureChecker
{
	static void CheckAttribute(BFile &file, const SignatureValue &value)
	{
		CheckAttr(file, kSignatureAttribute, B_MIME_STRING_TYPE, value.type.c_str(),
				  value.type.length() + 1);
	}

	static void CheckResource(BFile &file, const SignatureValue &value)
	{
		::CheckResource(file, kSignatureAttribute, kSignatureResourceID,
						B_MIME_STRING_TYPE, value.type.c_str(),
						value.type.length() + 1);
	}
};

// AppFlagsValue
struct AppFlagsValue
{
	AppFlagsValue() : flags() {}
	AppFlagsValue(uint32 flags) : flags(flags) {}

	bool operator==(const AppFlagsValue &value)
	{
		return (flags == value.flags);
	}

	uint32 flags;
};

// AppFlagsSetter
struct AppFlagsSetter
{
	static status_t Set(BAppFileInfo &info, const AppFlagsValue &value)
	{
		return info.SetAppFlags(value.flags);
	}
};

// AppFlagsGetter
struct AppFlagsGetter
{
	static status_t Get(BAppFileInfo &info, AppFlagsValue &value)
	{
		return info.GetAppFlags(&value.flags);
	}
};

// AppFlagsChecker
struct AppFlagsChecker
{
	static void CheckAttribute(BFile &file, const AppFlagsValue &value)
	{
		CheckAttr(file, kAppFlagsAttribute, APP_FLAGS_TYPE, &value.flags,
				  sizeof(value.flags));
	}

	static void CheckResource(BFile &file, const AppFlagsValue &value)
	{
		::CheckResource(file, kAppFlagsAttribute, kAppFlagsResourceID,
						APP_FLAGS_TYPE, &value.flags, sizeof(value.flags));
	}
};

// SupportedTypesValue
struct SupportedTypesValue
{
	SupportedTypesValue() : types() {}
	SupportedTypesValue(const BMessage &types) : types(types) {}
	SupportedTypesValue(const char **types, int32 count)
		: types()
	{
		for (int32 i = 0; i < count; i++)
			this->types.AddString("types", types[i]);
	}

	bool operator==(const SupportedTypesValue &value)
	{
		type_code type1, type2;
		int32 count1, count2;
		bool equal = (types.GetInfo("types", &type1, &count1) == B_OK
					  && value.types.GetInfo("types", &type2, &count2) == B_OK
					  && type1 == type2 && count1 == count2);
		
		for (int32 i = 0; equal && i < count1; i++) {
			BString str1, str2;
			equal = types.FindString("types", i, &str1) == B_OK
					&& value.types.FindString("types", i, &str2) == B_OK
					&& str1 == str2;
		}
		return equal;
	}

	BMessage types;
};

// SupportedTypesSetter
struct SupportedTypesSetter
{
	static status_t Set(BAppFileInfo &info, const SupportedTypesValue &value)
	{
		return info.SetSupportedTypes(&value.types, false);
	}
};

// SupportedTypesGetter
struct SupportedTypesGetter
{
	static status_t Get(BAppFileInfo &info, SupportedTypesValue &value)
	{
		return info.GetSupportedTypes(&value.types);
	}
};

// SupportedTypesChecker
struct SupportedTypesChecker
{
	static void CheckAttribute(BFile &file, const SupportedTypesValue &value)
	{
		size_t size;
		char *buffer = ReadAttr(file, kSupportedTypesAttribute,
								B_MESSAGE_TYPE, size);
		AutoDeleter<char> deleter(buffer, true);
		BMessage storedTypes;
		CHK(storedTypes.Unflatten(buffer) == B_OK);
		SupportedTypesValue storedValue(storedTypes);
		CHK(storedValue == value);
	}

	static void CheckResource(BFile &file, const SupportedTypesValue &value)
	{
		size_t size;
		char *buffer = ReadResource(file, kSupportedTypesAttribute,
									kSupportedTypesResourceID, B_MESSAGE_TYPE,
									size);
		AutoDeleter<char> deleter(buffer, true);
		BMessage storedTypes;
		CHK(storedTypes.Unflatten(buffer) == B_OK);
		SupportedTypesValue storedValue(storedTypes);
		CHK(storedValue == value);
	}
};

// IconValue
struct IconValue
{
	IconValue() : mini(BRect(0, 0, 15, 15), B_CMAP8),
				  large(BRect(0, 0, 31, 31), B_CMAP8) {}
	IconValue(const BBitmap *mini, const BBitmap *large)
		: mini(BRect(0, 0, 15, 15), B_CMAP8),
		  large(BRect(0, 0, 31, 31), B_CMAP8)
	{
		this->mini.SetBits(mini->Bits(), mini->BitsLength(), 0,
						   mini->ColorSpace());
		this->large.SetBits(large->Bits(), large->BitsLength(), 0,
							large->ColorSpace());
	}

	bool operator==(const IconValue &value)
	{
		return (icon_equal(&mini, &value.mini)
				&& icon_equal(&large, &value.large));
	}

	BBitmap mini;
	BBitmap large;
};

// IconSetter
struct IconSetter
{
	static status_t Set(BAppFileInfo &info, const IconValue &value)
	{
		status_t error = info.SetIcon(&value.mini, B_MINI_ICON);
		if (error == B_OK)
			error = info.SetIcon(&value.large, B_LARGE_ICON);
		return error;
	}
};

// IconGetter
struct IconGetter
{
	static status_t Get(BAppFileInfo &info, IconValue &value)
	{
		status_t error = info.GetIcon(&value.mini, B_MINI_ICON);
		if (error == B_OK)
			error = info.GetIcon(&value.large, B_LARGE_ICON);
		return error;
	}
};

// IconChecker
struct IconChecker
{
	static void CheckAttribute(BFile &file, const IconValue &value)
	{
		CheckAttr(file, kMiniIconAttribute, MINI_ICON_TYPE, value.mini.Bits(),
				  value.mini.BitsLength());
		CheckAttr(file, kLargeIconAttribute, LARGE_ICON_TYPE,
				  value.large.Bits(), value.large.BitsLength());
	}

	static void CheckResource(BFile &file, const IconValue &value)
	{
		::CheckResource(file, kMiniIconAttribute, kMiniIconResourceID,
						MINI_ICON_TYPE, value.mini.Bits(),
						value.mini.BitsLength());
		::CheckResource(file, kLargeIconAttribute, kLargeIconResourceID,
						LARGE_ICON_TYPE, value.large.Bits(),
						value.large.BitsLength());
	}
};

// VersionInfoValue
struct VersionInfoValue
{
	VersionInfoValue() : app(), system() {}
	VersionInfoValue(const version_info &app, const version_info &system)
		: app(app), system(system) {}

	bool operator==(const VersionInfoValue &value)
	{
		return (app == value.app && system == value.system);
	}

	version_info app;
	version_info system;
};

// VersionInfoSetter
struct VersionInfoSetter
{
	static status_t Set(BAppFileInfo &info, const VersionInfoValue &value)
	{
		status_t error = info.SetVersionInfo(&value.app, B_APP_VERSION_KIND);
		if (error == B_OK)
			error = info.SetVersionInfo(&value.system, B_SYSTEM_VERSION_KIND);
		return error;
	}
};

// VersionInfoGetter
struct VersionInfoGetter
{
	static status_t Get(BAppFileInfo &info, VersionInfoValue &value)
	{
		status_t error = info.GetVersionInfo(&value.app, B_APP_VERSION_KIND);
		if (error == B_OK)
			error = info.GetVersionInfo(&value.system, B_SYSTEM_VERSION_KIND);
		return error;
	}
};

// VersionInfoChecker
struct VersionInfoChecker
{
	static void CheckAttribute(BFile &file, const VersionInfoValue &value)
	{
		version_info infos[] = { value.app, value.system };
		CheckAttr(file, kVersionInfoAttribute, VERSION_INFO_TYPE,
				  infos, sizeof(infos));
	}

	static void CheckResource(BFile &file, const VersionInfoValue &value)
	{
		version_info infos[] = { value.app, value.system };
		::CheckResource(file, kVersionInfoAttribute,
						kVersionInfoResourceID, VERSION_INFO_TYPE,
						infos, sizeof(infos));
	}
};

// IconForTypeValue
struct IconForTypeValue : public IconValue
{
//	IconForTypeValue() : type() {}
//	IconForTypeValue(const BBitmap *mini, const BBitmap *large,
//					 const char *type) : IconValue(mini, large), type(type) {}
	IconForTypeValue() : type(testType1) {}
	IconForTypeValue(const BBitmap *mini, const BBitmap *large)
		: IconValue(mini, large), type(testType1) {}

	string	type;
};

// IconForTypeSetter
struct IconForTypeSetter
{
	static status_t Set(BAppFileInfo &info, const IconForTypeValue &value)
	{
		status_t error = info.SetIconForType(value.type.c_str(), &value.mini,
											 B_MINI_ICON);
		if (error == B_OK) {
			error = info.SetIconForType(value.type.c_str(), &value.large,
										B_LARGE_ICON);
		}
		return error;
	}
};

// IconForTypeGetter
struct IconForTypeGetter
{
	static status_t Get(BAppFileInfo &info, IconForTypeValue &value)
	{
		status_t error = info.GetIconForType(value.type.c_str(), &value.mini,
											 B_MINI_ICON);
		if (error == B_OK) {
			error = info.GetIconForType(value.type.c_str(), &value.large,
										B_LARGE_ICON);
		}
		return error;
	}
};

// IconForTypeChecker
struct IconForTypeChecker
{
	static void CheckAttribute(BFile &file, const IconForTypeValue &value)
	{
		string attrNameM(kMiniIconForTypeAttribute);
		attrNameM += value.type;
		CheckAttr(file, attrNameM.c_str(), MINI_ICON_TYPE, value.mini.Bits(),
				  value.mini.BitsLength());
		string attrNameL(kLargeIconForTypeAttribute);
		attrNameL += value.type;
		CheckAttr(file, attrNameL.c_str(), LARGE_ICON_TYPE,
				  value.large.Bits(), value.large.BitsLength());
	}

	static void CheckResource(BFile &file, const IconForTypeValue &value)
	{
		string attrNameM(kMiniIconForTypeAttribute);
		attrNameM += value.type;
		::CheckResource(file, attrNameM.c_str(), kMiniIconForTypeResourceID,
						MINI_ICON_TYPE, value.mini.Bits(),
						value.mini.BitsLength());
		string attrNameL(kLargeIconForTypeAttribute);
		attrNameL += value.type;
		::CheckResource(file, attrNameL.c_str(), kLargeIconForTypeResourceID,
						LARGE_ICON_TYPE, value.large.Bits(),
						value.large.BitsLength());
	}
};


// InfoLocationTest
void
AppFileInfoTest::InfoLocationTest()
{
	// tests:
	// * void SetInfoLocation(info_location location);
	// * bool IsUsingAttributes() const;
	// * bool IsUsingResources() const;

	// type
	NextSubTest();
	InfoLocationTester<TypeValue, TypeSetter, TypeGetter, TypeChecker>(
		TypeValue(testType1), TypeValue(testType2), TypeValue(testType3));

	// signature
	NextSubTest();
	InfoLocationTester<SignatureValue, SignatureSetter, SignatureGetter,
					   SignatureChecker>(
		SignatureValue(testAppSignature1), SignatureValue(testAppSignature2),
		SignatureValue(testAppSignature3));

	// app flags
	NextSubTest();
	InfoLocationTester<AppFlagsValue, AppFlagsSetter, AppFlagsGetter,
					   AppFlagsChecker>(
		AppFlagsValue(B_SINGLE_LAUNCH | B_ARGV_ONLY),
		AppFlagsValue(B_EXCLUSIVE_LAUNCH | B_BACKGROUND_APP),
		AppFlagsValue(B_MULTIPLE_LAUNCH));

	// supported types
	NextSubTest();
	// The file needs to have a signature and the signature must be known
	// to the MIME database. Otherwise SetSupportedType() fails.
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		BMimeType type(testAppSignature1);
		if (!type.IsInstalled())
			CHK(type.Install() == B_OK);
	}
	const char *supportedTypes1[] = { testType1 };
	const char *supportedTypes2[] = { testType2, testType3 };
	const char *supportedTypes3[] = { testType3, testType2, testType1 };
	InfoLocationTester<SupportedTypesValue, SupportedTypesSetter,
					   SupportedTypesGetter, SupportedTypesChecker>(
		SupportedTypesValue(supportedTypes1, 1),
		SupportedTypesValue(supportedTypes2, 2),
		SupportedTypesValue(supportedTypes3, 3));

	// icon
	NextSubTest();
	InfoLocationTester<IconValue, IconSetter, IconGetter, IconChecker>(
		IconValue(fIconM1, fIconL1),
		IconValue(fIconM2, fIconL2),
		IconValue(fIconM3, fIconL3));

	// version info
	NextSubTest();
	version_info versionInfo1 = { 1, 1, 1, 1, 1, "short1", "long1" };
	version_info versionInfo2 = { 2, 2, 2, 2, 2, "short2", "long2" };
	version_info versionInfo3 = { 3, 3, 3, 3, 3, "short3", "long3" };
	InfoLocationTester<VersionInfoValue, VersionInfoSetter, VersionInfoGetter,
					   VersionInfoChecker>(
		VersionInfoValue(versionInfo1, versionInfo1),
		VersionInfoValue(versionInfo2, versionInfo2),
		VersionInfoValue(versionInfo3, versionInfo3));

	// icon for type
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		BMimeType type(testAppSignature1);
		if (!type.IsInstalled())
			CHK(type.Install() == B_OK);
		BMessage supportedTypes;
		supportedTypes.AddString("types", testType1);
		CHK(appFileInfo.SetSupportedTypes(&supportedTypes) == B_OK);
		CHK(type.SetTo(testType1) == B_OK);
		CHK(type.SetPreferredApp(testAppSignature1) == B_OK);
	}
	InfoLocationTester<IconForTypeValue, IconForTypeSetter, IconForTypeGetter,
					   IconForTypeChecker>(
		IconForTypeValue(fIconM1, fIconL1),
		IconForTypeValue(fIconM2, fIconL2),
		IconForTypeValue(fIconM3, fIconL3));
}

