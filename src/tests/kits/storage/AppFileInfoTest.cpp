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
static const char *testType1
	= "application/x-vnd.obos.app-file-info-test1";
static const char *testType2
	= "application/x-vnd.obos.app-file-info-test2";
static const char *testType3
	= "application/x-vnd.obos.app-file-info-test3";
static const char *testType4
	= "application/x-vnd.obos.app-file-info-test4";
static const char *invalidTestType	= "invalid/mime/type";
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
		testType1, testType2, testType3, testType4,
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

// SyncResources
static
void
SyncResources(BAppFileInfo &info)
{
	struct node_info_hack {
		virtual ~node_info_hack() {}
		BNode		*fNode;
		uint32		_reserved[2];
		status_t	fCStatus;
	};
	struct app_file_info_hack : node_info_hack {
		virtual ~app_file_info_hack() {}
		BResources		*fResources;
		info_location	fWhere;
		uint32			_reserved[2];
	};
	app_file_info_hack &hackedInfo
		= reinterpret_cast<app_file_info_hack&>(info);
	BResources *resources = hackedInfo.fResources;
	if (resources)
		resources->Sync();
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

// CheckNoResource
static
void
CheckNoResource(BFile &file, const char *name)
{
	BResources resources;
	CHK(resources.SetTo(&file) == B_OK);
	type_code typeFound;
	int32 idFound;
	const char *nameFound;
	size_t size;
	bool found = false;
	for (int32 i = 0;
		 !found && resources.GetResourceInfo(i, &typeFound, &idFound,
											 &nameFound, &size);
		 i++) {
		found = !strcmp(nameFound, name);
	}
	CHK(!found);
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
	static void CheckAttribute(BNode &file, const TypeValue &value)
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
	static void CheckAttribute(BNode &file, const SignatureValue &value)
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
	static void CheckAttribute(BNode &file, const AppFlagsValue &value)
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
	static void CheckAttribute(BNode &file, const SupportedTypesValue &value)
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
	static void CheckAttribute(BNode &file, const IconValue &value)
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
	static void CheckAttribute(BNode &file, const VersionInfoValue &value)
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
	static void CheckAttribute(BNode &file, const IconForTypeValue &value)
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


// CheckTypeAttr
static
void
CheckTypeAttr(BNode &node, const char *data)
{
	CheckAttr(node, kTypeAttribute, B_MIME_STRING_TYPE, data,
			  strlen(data) + 1);
}

// CheckTypeResource
static
void
CheckTypeResource(BFile &file, const char *data)
{
	CheckResource(file, kTypeAttribute, kTypeResourceID, B_MIME_STRING_TYPE,
				  data, strlen(data) + 1);
}

// CheckSignatureAttr
static
void
CheckSignatureAttr(BNode &node, const char *data)
{
	CheckAttr(node, kSignatureAttribute, B_MIME_STRING_TYPE, data,
			  strlen(data) + 1);
}

// CheckSignatureResource
static
void
CheckSignatureResource(BFile &file, const char *data)
{
	CheckResource(file, kSignatureAttribute, kSignatureResourceID,
				  B_MIME_STRING_TYPE, data, strlen(data) + 1);
}

// CheckAppFlagsAttr
static
void
CheckAppFlagsAttr(BNode &node, uint32 flags)
{
	CheckAttr(node, kAppFlagsAttribute, APP_FLAGS_TYPE, &flags, sizeof(flags));
}

// CheckAppFlagsResource
static
void
CheckAppFlagsResource(BFile &file, uint32 flags)
{
	CheckResource(file, kAppFlagsAttribute, kAppFlagsResourceID,
				  APP_FLAGS_TYPE, &flags, sizeof(flags));
}

// CheckSupportedTypesAttr
static
void
CheckSupportedTypesAttr(BNode &node, const BMessage *data)
{
	SupportedTypesChecker::CheckAttribute(node, SupportedTypesValue(*data));
}

// CheckSupportedTypesResource
static
void
CheckSupportedTypesResource(BFile &file, const BMessage *data)
{
	SupportedTypesChecker::CheckResource(file, SupportedTypesValue(*data));
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

// CheckIconResource
static
void
CheckIconResource(BFile &file, BBitmap *data)
{
	const char *attribute = NULL;
	int32 resourceID = 0;
	uint32 type = 0;
	switch (data->Bounds().IntegerWidth()) {
		case 15:
			attribute = kMiniIconAttribute;
			resourceID = kMiniIconResourceID;
			type = MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconAttribute;
			resourceID = kLargeIconResourceID;
			type = LARGE_ICON_TYPE;
			break;
		default:
			CHK(false);
			break;
	}
	CheckResource(file, attribute, resourceID, type, data->Bits(),
				  data->BitsLength());
}

// CheckVersionInfoAttr
static
void
CheckVersionInfoAttr(BNode &node, version_info *data)
{
	CheckAttr(node, kVersionInfoAttribute, VERSION_INFO_TYPE, data,
			  2 * sizeof(version_info));
}

// CheckVersionInfoResource
static
void
CheckVersionInfoResource(BFile &file, version_info *data)
{
	CheckResource(file, kVersionInfoAttribute, kVersionInfoResourceID,
				  VERSION_INFO_TYPE, data, 2 * sizeof(version_info));
}

// CheckIconForTypeAttr
static
void
CheckIconForTypeAttr(BNode &node, const char *type, BBitmap *data)
{
	string attribute;
	int32 resourceID = 0;
	uint32 iconType = 0;
	switch (data->Bounds().IntegerWidth()) {
		case 15:
			attribute = kMiniIconForTypeAttribute;
			resourceID = kMiniIconForTypeResourceID;
			iconType = MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconForTypeAttribute;
			resourceID = kLargeIconForTypeResourceID;
			iconType = LARGE_ICON_TYPE;
			break;
		default:
			CHK(false);
			break;
	}
	attribute += type;
	CheckAttr(node, attribute.c_str(), iconType, data->Bits(),
			  data->BitsLength());
}

// CheckIconForTypeResource
static
void
CheckIconForTypeResource(BFile &file, const char *type, BBitmap *data,
						 int32 resourceID = -1)
{
	string attribute;
	uint32 iconType = 0;
	switch (data->Bounds().IntegerWidth()) {
		case 15:
			attribute = kMiniIconForTypeAttribute;
			if (resourceID < 0)
				resourceID = kMiniIconForTypeResourceID;
			iconType = MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconForTypeAttribute;
			if (resourceID < 0)
				resourceID = kLargeIconForTypeResourceID;
			iconType = LARGE_ICON_TYPE;
			break;
		default:
			CHK(false);
			break;
	}
	attribute += type;
	CheckResource(file, attribute.c_str(), resourceID, iconType, data->Bits(),
				  data->BitsLength());
}

// CheckNoIconForTypeAttr
static
void
CheckNoIconForTypeAttr(BNode &node, const char *type, icon_size iconSize)
{
	string attribute;
	switch (iconSize) {
		case B_MINI_ICON:
			attribute = kMiniIconForTypeAttribute;
			break;
		case B_LARGE_ICON:
			attribute = kLargeIconForTypeAttribute;
			break;
		default:
			CHK(false);
			break;
	}
	attribute += type;
	CheckNoAttr(node, attribute.c_str());
}

// CheckNoIconForTypeResource
static
void
CheckNoIconForTypeResource(BFile &file, const char *type, icon_size iconSize)
{
	string attribute;
	switch (iconSize) {
		case B_MINI_ICON:
			attribute = kMiniIconForTypeAttribute;
			break;
		case B_LARGE_ICON:
			attribute = kLargeIconForTypeAttribute;
			break;
		default:
			CHK(false);
			break;
	}
	attribute += type;
	CheckNoResource(file, attribute.c_str());
}


// InitTest1
void
AppFileInfoTest::InitTest1()
{
#ifdef TEST_R5
	const bool hasInitialLocation = false;
#else
	const bool hasInitialLocation = true;
#endif
	// BAppFileInfo()
	// * InitCheck() == B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.InitCheck() == B_NO_INIT);
		CHK(appFileInfo.IsUsingAttributes() == hasInitialLocation);
		CHK(appFileInfo.IsUsingResources() == hasInitialLocation);
	}

	// BAppFileInfo(BFile *file)
	// * NULL file => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BAppFileInfo appFileInfo(NULL);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == hasInitialLocation);
		CHK(appFileInfo.IsUsingResources() == hasInitialLocation);
	}
	// * invalid file => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BFile file;
		BAppFileInfo appFileInfo(&file);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == hasInitialLocation);
		CHK(appFileInfo.IsUsingResources() == hasInitialLocation);
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
#ifdef TEST_R5
	const bool hasInitialLocation = false;
#else
	const bool hasInitialLocation = true;
#endif
	// status_t SetTo(BFile *file)
	// * NULL file => InitCheck() == B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(NULL) == B_BAD_VALUE);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == hasInitialLocation);
		CHK(appFileInfo.IsUsingResources() == hasInitialLocation);
	}
	// * invalid file => InitCheck() == B_BAD_VALUE
	NextSubTest();
	{
		BFile file;
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_BAD_VALUE);
		CHK(appFileInfo.InitCheck() == B_BAD_VALUE);
		CHK(appFileInfo.IsUsingAttributes() == hasInitialLocation);
		CHK(appFileInfo.IsUsingResources() == hasInitialLocation);
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
	// status_t GetType(char *type) const
	// * NULL type => B_BAD_ADDRESS/B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(equals(appFileInfo.GetType(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		char type[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetType(type) == B_NO_INIT);
	}
	// * has no type => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		char type[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetType(type) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set
		CHK(appFileInfo.SetType(testType1) == B_OK);
		// get
		char type[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetType(type) == B_OK);
		CHK(strcmp(testType1, type) == 0);
		CheckTypeAttr(file, testType1);
		SyncResources(appFileInfo);
		CheckTypeResource(file, testType1);
		// reset
		CHK(appFileInfo.SetType(testType2) == B_OK);
		// get
		CHK(appFileInfo.GetType(type) == B_OK);
		CHK(strcmp(testType2, type) == 0);
		CheckTypeAttr(file, testType2);
		SyncResources(appFileInfo);
		CheckTypeResource(file, testType2);
	}

	// status_t SetType(const char *type)
	// * NULL type => B_OK, unsets the type
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetType(NULL) == B_OK);
		// get
		char type[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetType(type) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(file, kTypeAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kTypeAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetType(testType1) == B_NO_INIT);
	}
	// * invalid MIME type => B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetType(invalidTestType) == B_OK);
		// get
		char type[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetType(type) == B_OK);
		CHK(strcmp(invalidTestType, type) == 0);
		CheckTypeAttr(file, invalidTestType);
		SyncResources(appFileInfo);
		CheckTypeResource(file, invalidTestType);
	}
	// * type string too long => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// remove attr first
		CHK(appFileInfo.SetType(NULL) == B_OK);
		// try to set the too long type
		CHK(appFileInfo.SetType(tooLongTestType) == B_BAD_VALUE);
		CheckNoAttr(file, kTypeAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kTypeAttribute);
	}
}

// SignatureTest
void
AppFileInfoTest::SignatureTest()
{
	// status_t GetSignature(char *signature) const
	// * NULL signature => B_BAD_ADDRESS/B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(equals(appFileInfo.GetSignature(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		char signature[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetSignature(signature) == B_NO_INIT);
	}
	// * has no signature => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		char signature[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetSignature(signature) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		CHK(BMimeType(testAppSignature1).IsInstalled() == false);
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetSignature(signature) == B_OK);
		CHK(strcmp(testAppSignature1, signature) == 0);
		CheckSignatureAttr(file, testAppSignature1);
		SyncResources(appFileInfo);
		CheckSignatureResource(file, testAppSignature1);
		// reset
		CHK(appFileInfo.SetSignature(testAppSignature2) == B_OK);
		CHK(BMimeType(testAppSignature2).IsInstalled() == false);
		// get
		CHK(appFileInfo.GetSignature(signature) == B_OK);
		CHK(strcmp(testAppSignature2, signature) == 0);
		CheckSignatureAttr(file, testAppSignature2);
		SyncResources(appFileInfo);
		CheckSignatureResource(file, testAppSignature2);
	}

	// status_t SetSignature(const char *signature)
	// * NULL signature => B_OK, unsets the signature
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(NULL) == B_OK);
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetSignature(signature) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(file, kSignatureAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSignatureAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_NO_INIT);
	}
	// * invalid MIME signature => B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(invalidTestType) == B_OK);
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CHK(appFileInfo.GetSignature(signature) == B_OK);
		CHK(strcmp(invalidTestType, signature) == 0);
		CheckSignatureAttr(file, invalidTestType);
		SyncResources(appFileInfo);
		CheckSignatureResource(file, invalidTestType);
	}
	// * signature string too long => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// remove attr first
		CHK(appFileInfo.SetSignature(NULL) == B_OK);
		// try to set the too long signature
		CHK(appFileInfo.SetSignature(tooLongTestType) == B_BAD_VALUE);
		CheckNoAttr(file, kSignatureAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSignatureAttribute);
	}
}

// AppFlagsTest
void
AppFileInfoTest::AppFlagsTest()
{
	// status_t GetAppFlags(uint32 *flags) const
	// * NULL flags => B_BAD_ADDRESS/B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(equals(appFileInfo.GetAppFlags(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		uint32 flags;
		CHK(appFileInfo.GetAppFlags(&flags) == B_NO_INIT);
	}
	// * has no flags => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		uint32 flags;
		CHK(appFileInfo.GetAppFlags(&flags) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		uint32 testFlags1 = B_SINGLE_LAUNCH | B_BACKGROUND_APP;
		uint32 testFlags2 = B_MULTIPLE_LAUNCH | B_ARGV_ONLY;
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set
		CHK(appFileInfo.SetAppFlags(testFlags1) == B_OK);
		// get
		uint32 flags;
		CHK(appFileInfo.GetAppFlags(&flags) == B_OK);
		CHK(flags == testFlags1);
		CheckAppFlagsAttr(file, testFlags1);
		SyncResources(appFileInfo);
		CheckAppFlagsResource(file, testFlags1);
		// reset
		CHK(appFileInfo.SetAppFlags(testFlags2) == B_OK);
		// get
		CHK(appFileInfo.GetAppFlags(&flags) == B_OK);
		CHK(flags == testFlags2);
		CheckAppFlagsAttr(file, testFlags2);
		SyncResources(appFileInfo);
		CheckAppFlagsResource(file, testFlags2);
	}

	// status_t SetAppFlags(uint32 flags)
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetAppFlags(B_SINGLE_LAUNCH) == B_NO_INIT);
	}
}

// IsSupportingApp
static
bool
IsSupportingApp(const char *type, const char *signature)
{
	BMessage apps;
//	CHK(BMimeType(type).GetSupportingApps(&apps) == B_OK);
	BMimeType(type).GetSupportingApps(&apps);
	bool found = false;
	BString app;
	for (int32 i = 0;
		 !found && apps.FindString("applications", i, &app) == B_OK;
		 i++) {
		found = (app == signature);
	}
	return found;
}

// SupportedTypesTest
void
AppFileInfoTest::SupportedTypesTest()
{
	// test data
	BMessage testTypes1;
	CHK(testTypes1.AddString("types", testType1) == B_OK);
	CHK(testTypes1.AddString("types", testType2) == B_OK);
	BMessage testTypes2;
	CHK(testTypes2.AddString("types", testType3) == B_OK);
	CHK(testTypes2.AddString("types", testType4) == B_OK);
	BMimeType mimeTestType1(testType1);
	BMimeType mimeTestType2(testType2);
	BMimeType mimeTestType3(testType3);
	BMimeType mimeTestType4(testType4);
	// add and install the app signature
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		CHK(BMimeType(testAppSignature1).Install() == B_OK);
	}

	// status_t GetSupportedTypes(BMessage *types) const;
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_NO_INIT);
	}
	// * has no supported types => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_OK);
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_OK);
		CHK(SupportedTypesValue(types) == SupportedTypesValue(testTypes1));
		CHK(appFileInfo.IsSupportedType(testType1) == true);
		CHK(appFileInfo.IsSupportedType(testType2) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == true);
		CHK(appFileInfo.Supports(&mimeTestType2) == true);
		CheckSupportedTypesAttr(file, &testTypes1);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes1);
		CHK(BMimeType(testType1).IsInstalled() == true);
		CHK(BMimeType(testType2).IsInstalled() == true);
		// reset
		CHK(BMimeType(testType3).IsInstalled() == false);
		CHK(BMimeType(testType4).IsInstalled() == false);
		CHK(appFileInfo.SetSupportedTypes(&testTypes2) == B_OK);
		// get
		BMessage types2;
		CHK(appFileInfo.GetSupportedTypes(&types2) == B_OK);
		CHK(SupportedTypesValue(types2) == SupportedTypesValue(testTypes2));
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.IsSupportedType(testType3) == true);
		CHK(appFileInfo.IsSupportedType(testType4) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType3) == true);
		CHK(appFileInfo.Supports(&mimeTestType4) == true);
		CheckSupportedTypesAttr(file, &testTypes2);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes2);
		CHK(BMimeType(testType3).IsInstalled() == true);
		CHK(BMimeType(testType4).IsInstalled() == true);
	}
	// * partially filled types => types is cleared beforehand
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BMessage types2;
		CHK(types2.AddString("types", testType1) == B_OK);
		CHK(types2.AddString("dummy", "Hello") == B_OK);
		CHK(appFileInfo.GetSupportedTypes(&types2) == B_OK);
		CHK(SupportedTypesValue(types2) == SupportedTypesValue(testTypes2));
		const char *dummy;
		CHK(types2.FindString("dummy", &dummy) != B_OK);
	}
	// * NULL types => B_BAD_VALUE
// R5: crashes when passing NULL types
#ifndef TEST_R5
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.GetSupportedTypes(NULL) == B_BAD_VALUE);
	}
#endif

	// status_t SetSupportedTypes(const BMessage *types, bool syncAll);
	// status_t SetSupportedTypes(const BMessage *types);
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		BMessage types;
		CHK(appFileInfo.SetSupportedTypes(&types) == B_NO_INIT);
	}
	// * NULL types => unsets supported types
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set
		CHK(appFileInfo.SetSupportedTypes(NULL) == B_OK);
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.IsSupportedType(testType3) == false);
		CHK(appFileInfo.IsSupportedType(testType4) == false);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType3) == false);
		CHK(appFileInfo.Supports(&mimeTestType4) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		// set: syncAll = true
// R5: crashes when passing NULL types
#ifndef TEST_R5
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_OK);
		CHK(appFileInfo.SetSupportedTypes(NULL, true) == B_OK);
		// get
		CHK(appFileInfo.GetSupportedTypes(&types) == B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.IsSupportedType(testType3) == false);
		CHK(appFileInfo.IsSupportedType(testType4) == false);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType3) == false);
		CHK(appFileInfo.Supports(&mimeTestType4) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
#endif
	}
	// * cross check with BMimeType::GetSupportingApps(): no syncAll
	NextSubTest();
	{
		// clean up
		CHK(BMimeType(testType1).Delete() == B_OK);
		CHK(BMimeType(testType2).Delete() == B_OK);
		CHK(BMimeType(testType3).Delete() == B_OK);
		CHK(BMimeType(testType4).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Install() == B_OK);
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// set
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_OK);
		// get
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
		// reset
		CHK(appFileInfo.SetSupportedTypes(&testTypes2) == B_OK);
		// get
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
		CHK(IsSupportingApp(testType3, testAppSignature1) == true);
		CHK(IsSupportingApp(testType4, testAppSignature1) == true);
	}
	// * cross check with BMimeType::GetSupportingApps(): syncAll == false
	NextSubTest();
	{
		// clean up
		CHK(BMimeType(testType1).Delete() == B_OK);
		CHK(BMimeType(testType2).Delete() == B_OK);
		CHK(BMimeType(testType3).Delete() == B_OK);
		CHK(BMimeType(testType4).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Install() == B_OK);
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// set
		CHK(appFileInfo.SetSupportedTypes(&testTypes1, false) == B_OK);
		// get
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
		// reset
		CHK(appFileInfo.SetSupportedTypes(&testTypes2, false) == B_OK);
		// get
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
		CHK(IsSupportingApp(testType3, testAppSignature1) == true);
		CHK(IsSupportingApp(testType4, testAppSignature1) == true);
	}
	// * cross check with BMimeType::GetSupportingApps(): syncAll == true
	NextSubTest();
	{
		// clean up
		CHK(BMimeType(testType1).Delete() == B_OK);
		CHK(BMimeType(testType2).Delete() == B_OK);
		CHK(BMimeType(testType3).Delete() == B_OK);
		CHK(BMimeType(testType4).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Install() == B_OK);
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// set
		CHK(appFileInfo.SetSupportedTypes(&testTypes1, true) == B_OK);
		// get
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
		// reset
		CHK(appFileInfo.SetSupportedTypes(&testTypes2, true) == B_OK);
		// get
		CHK(IsSupportingApp(testType1, testAppSignature1) == false);
		CHK(IsSupportingApp(testType2, testAppSignature1) == false);
		CHK(IsSupportingApp(testType3, testAppSignature1) == true);
		CHK(IsSupportingApp(testType4, testAppSignature1) == true);
	}
	// * types contains invalid MIME types
// R5: doesn't fail
#ifndef TEST_R5
	NextSubTest();
	{
		// init
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set
		BMessage invalidTestTypes;
		CHK(invalidTestTypes.AddString("types", invalidTestType) == B_OK);
		CHK(appFileInfo.SetSupportedTypes(NULL, true) == B_OK);
		CHK(appFileInfo.SetSupportedTypes(&invalidTestTypes, false)
			== B_BAD_VALUE);
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.IsSupportedType(invalidTestType) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
	}
#endif

	// bool IsSupportedType(const char *type) const;
	// bool Supports(BMimeType *type) const;
	// * NULL type => false
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.IsSupportedType(NULL) == false);
// R5: crashes when passing a NULL type
#ifndef TEST_R5
		CHK(appFileInfo.Supports(NULL) == false);
#endif
	}
	// * supports "application/octet-stream"
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BMimeType gifType("image/gif");
		CHK(appFileInfo.IsSupportedType(gifType.Type()) == false);
		CHK(appFileInfo.Supports(&gifType) == false);
		BMessage types;
		CHK(types.AddString("types", "application/octet-stream") == B_OK);
		CHK(appFileInfo.SetSupportedTypes(&types) == B_OK);
		CHK(appFileInfo.IsSupportedType(gifType.Type()) == true);
		CHK(appFileInfo.Supports(&gifType) == false);
		BMessage noTypes;
		CHK(appFileInfo.SetSupportedTypes(&noTypes, true) == B_OK);
	}

	// various
	// * signature not installed, syncAll = true
	NextSubTest();
	{
		// clean up
		CHK(BMimeType(testType1).Delete() == B_OK);
		CHK(BMimeType(testType2).Delete() == B_OK);
		CHK(BMimeType(testType3).Delete() == B_OK);
		CHK(BMimeType(testType4).Delete() == B_OK);
		CHK(BMimeType(testAppSignature1).Delete() == B_OK);
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// set
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		CHK(appFileInfo.SetSupportedTypes(&testTypes1, true) == B_OK);
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_OK);
		CHK(SupportedTypesValue(types) == SupportedTypesValue(testTypes1));
		CHK(appFileInfo.IsSupportedType(testType1) == true);
		CHK(appFileInfo.IsSupportedType(testType2) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == true);
		CHK(appFileInfo.Supports(&mimeTestType2) == true);
		CheckSupportedTypesAttr(file, &testTypes1);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes1);
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		// set
		CHK(BMimeType(testType3).IsInstalled() == false);
		CHK(BMimeType(testType4).IsInstalled() == false);
		CHK(appFileInfo.SetSupportedTypes(&testTypes2, true) == B_OK);
		// get
		CHK(appFileInfo.GetSupportedTypes(&types) == B_OK);
		CHK(SupportedTypesValue(types) == SupportedTypesValue(testTypes2));
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.IsSupportedType(testType3) == true);
		CHK(appFileInfo.IsSupportedType(testType4) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType3) == true);
		CHK(appFileInfo.Supports(&mimeTestType4) == true);
		CheckSupportedTypesAttr(file, &testTypes2);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes2);
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		CHK(BMimeType(testType3).IsInstalled() == false);
// R5: returns true. In fact the last SetSupportedTypes() installed testType4
// in the database, but not testType3. Certainly a bug
#ifndef TEST_R5
		CHK(BMimeType(testType4).IsInstalled() == false);
#endif
	}
	// * signature not installed, no syncAll
	NextSubTest();
	{
		// clean up
		BMimeType(testType1).Delete();
		BMimeType(testType2).Delete();
		BMimeType(testType3).Delete();
		BMimeType(testType4).Delete();
		BMimeType(testAppSignature1).Delete();
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// set
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
// R5: SetSupportedTypes() returns B_ENTRY_NOT_FOUND, although it does not
//     fail.
#ifdef TEST_R5
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_ENTRY_NOT_FOUND);
#else
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_OK);
#endif
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_OK);
		CHK(SupportedTypesValue(types) == SupportedTypesValue(testTypes1));
		CHK(appFileInfo.IsSupportedType(testType1) == true);
		CHK(appFileInfo.IsSupportedType(testType2) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == true);
		CHK(appFileInfo.Supports(&mimeTestType2) == true);
		CheckSupportedTypesAttr(file, &testTypes1);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes1);
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		// set
		CHK(BMimeType(testType3).IsInstalled() == false);
		CHK(BMimeType(testType4).IsInstalled() == false);
// R5: SetSupportedTypes() returns B_ENTRY_NOT_FOUND, although it does not
//     fail.
#ifdef TEST_R5
		CHK(appFileInfo.SetSupportedTypes(&testTypes2) == B_ENTRY_NOT_FOUND);
#else
		CHK(appFileInfo.SetSupportedTypes(&testTypes2) == B_OK);
#endif
		// get
		CHK(appFileInfo.GetSupportedTypes(&types) == B_OK);
		CHK(SupportedTypesValue(types) == SupportedTypesValue(testTypes2));
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.IsSupportedType(testType3) == true);
		CHK(appFileInfo.IsSupportedType(testType4) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType3) == true);
		CHK(appFileInfo.Supports(&mimeTestType4) == true);
		CheckSupportedTypesAttr(file, &testTypes2);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes2);
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		CHK(BMimeType(testType3).IsInstalled() == false);
// R5: returns true. In fact the last SetSupportedTypes() installed testType4
// in the database, but not testType3. Certainly a bug
#ifndef TEST_R5
		CHK(BMimeType(testType4).IsInstalled() == false);
#endif
	}
	// * no signature
	NextSubTest();
	{
		// clean up
		BMimeType(testType1).Delete();
		BMimeType(testType2).Delete();
		BMimeType(testType3).Delete();
		BMimeType(testType4).Delete();
		BMimeType(testAppSignature1).Delete();
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set, syncAll = true
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		CHK(appFileInfo.SetSupportedTypes(&testTypes1, true)
			== B_ENTRY_NOT_FOUND);
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		// set, no syncAll
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_ENTRY_NOT_FOUND);
		// get
		CHK(appFileInfo.GetSupportedTypes(&types) == B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		CHK(BMimeType(testType1).IsInstalled() == false);
		CHK(BMimeType(testType2).IsInstalled() == false);
	}
	// * set supported types, remove file, create file, get supported types
	NextSubTest();
	{
		// clean up
		BMimeType(testType1).Delete();
		BMimeType(testType2).Delete();
		BMimeType(testType3).Delete();
		BMimeType(testType4).Delete();
		BMimeType(testAppSignature1).Delete();
		CHK(BMimeType(testAppSignature1).Install() == B_OK);
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		BFile file(testFile1, B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// set
		CHK(appFileInfo.SetSupportedTypes(&testTypes1) == B_OK);
		// get
		BMessage types;
		CHK(appFileInfo.GetSupportedTypes(&types) == B_OK);
		CHK(SupportedTypesValue(types) == SupportedTypesValue(testTypes1));
		CHK(appFileInfo.IsSupportedType(testType1) == true);
		CHK(appFileInfo.IsSupportedType(testType2) == true);
		CHK(appFileInfo.Supports(&mimeTestType1) == true);
		CHK(appFileInfo.Supports(&mimeTestType2) == true);
		CheckSupportedTypesAttr(file, &testTypes1);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &testTypes1);
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
		// remove the file
		appFileInfo.SetTo(NULL);
		file.Unset();
		CHK(BEntry(testFile1).Remove() == B_OK);
		// init
		CHK(file.SetTo(testFile1, B_READ_WRITE | B_CREATE_FILE) == B_OK);
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		// get
		BMessage types2;
		CHK(appFileInfo.GetSupportedTypes(&types2) == B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.IsSupportedType(testType1) == false);
		CHK(appFileInfo.IsSupportedType(testType2) == false);
		CHK(appFileInfo.Supports(&mimeTestType1) == false);
		CHK(appFileInfo.Supports(&mimeTestType2) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		CHK(IsSupportingApp(testType1, testAppSignature1) == true);
		CHK(IsSupportingApp(testType2, testAppSignature1) == true);
	}
}

// IconTest
void
AppFileInfoTest::IconTest()
{
	// status_t GetIcon(BBitmap *icon, icon_size k) const
	// * NULL icon => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.GetIcon(NULL, B_MINI_ICON) == B_BAD_VALUE);
		CHK(appFileInfo.GetIcon(NULL, B_LARGE_ICON) == B_BAD_VALUE);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon2, B_LARGE_ICON) == B_NO_INIT);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon2, B_LARGE_ICON) == B_BAD_VALUE);
	}
	// * has no icon => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// mini
		// set
		CHK(appFileInfo.SetIcon(fIconM1, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM1, &icon));
		CheckIconAttr(file, fIconM1);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM1);
		// reset
		CHK(appFileInfo.SetIcon(fIconM2, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon2, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM2, &icon2));
		CheckIconAttr(file, fIconM2);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM2);
		// large
		// set
		CHK(appFileInfo.SetIcon(fIconL1, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon3, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL1, &icon3));
		CheckIconAttr(file, fIconL1);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL1);
		// reset
		CHK(appFileInfo.SetIcon(fIconL2, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon4, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL2, &icon4));
		CheckIconAttr(file, fIconL2);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL2);
	}
	// * bitmap color_space != B_CMAP8 => B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// mini
		BBitmap icon(BRect(0, 0, 15, 15), B_RGB32);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_OK);
		BBitmap icon2(BRect(0, 0, 15, 15), B_RGB32);
		// SetBits() can be used, since there's no row padding for 16x16.
		icon2.SetBits(fIconM2->Bits(), fIconM2->BitsLength(), 0, B_CMAP8);
		CHK(icon_equal(&icon, &icon2));
		// large
// R5: Crashes for some weird reason in GetIcon().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CHK(appFileInfo.GetIcon(&icon3, B_LARGE_ICON) == B_OK);
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
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// mini
		// set
		CHK(appFileInfo.SetIcon(NULL, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(file, kMiniIconAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kMiniIconAttribute);
		// large
		// set
		CHK(appFileInfo.SetIcon(NULL, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon2, B_LARGE_ICON) == B_ENTRY_NOT_FOUND);
		CheckNoAttr(file, kLargeIconAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kLargeIconAttribute);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.SetIcon(&icon, B_MINI_ICON) == B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.SetIcon(&icon2, B_LARGE_ICON) == B_NO_INIT);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.SetIcon(&icon, B_MINI_ICON) == B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.SetIcon(&icon2, B_LARGE_ICON) == B_BAD_VALUE);
	}
	// * file has app signature, set an icon => the icon is also set on the
	//   MIME type
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// uninstalled signature
		BMimeType mimeType(testAppSignature1);
		CHK(mimeType.IsInstalled() == false);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		CHK(appFileInfo.SetIcon(fIconM1, B_MINI_ICON) == B_OK);
		CHK(appFileInfo.SetIcon(fIconL1, B_LARGE_ICON) == B_OK);
		CHK(mimeType.IsInstalled() == true);
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(mimeType.GetIcon(&icon, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM1, &icon));
		// get large
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(mimeType.GetIcon(&icon2, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL1, &icon2));
		// installed signature
		CHK(appFileInfo.SetIcon(fIconM2, B_MINI_ICON) == B_OK);
		CHK(appFileInfo.SetIcon(fIconL2, B_LARGE_ICON) == B_OK);
		// get mini
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(mimeType.GetIcon(&icon3, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM2, &icon3));
		// get large
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(mimeType.GetIcon(&icon4, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL2, &icon4));
	}
}

// VersionInfoTest
void
AppFileInfoTest::VersionInfoTest()
{
//	status_t GetVersionInfo(version_info *info, version_kind kind) const;
//	status_t SetVersionInfo(const version_info *info, version_kind kind);
	version_info testInfo1 = { 1, 1, 1, 1, 1, "short1", "long1" };
	version_info testInfo2 = { 2, 2, 2, 2, 2, "short2", "long2" };
	version_info testInfo3 = { 3, 3, 3, 3, 3, "short3", "long3" };
	version_info testInfo4 = { 4, 4, 4, 4, 4, "short4", "long4" };


	// status_t GetVersionInfo(version_info *info, version_kind kind) const
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		version_info info;
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND)
			== B_NO_INIT);
		version_info info2;
		CHK(appFileInfo.GetVersionInfo(&info2, B_SYSTEM_VERSION_KIND)
			== B_NO_INIT);
	}
	// * has no version info => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		version_info info;
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND)
			== B_ENTRY_NOT_FOUND);
		version_info info2;
		CHK(appFileInfo.GetVersionInfo(&info2, B_SYSTEM_VERSION_KIND)
			== B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set app info
		CHK(appFileInfo.SetVersionInfo(&testInfo1, B_APP_VERSION_KIND)
			== B_OK);
		// get both infos
		version_info info;
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND) == B_OK);
		CHK(info == testInfo1);
		CHK(appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND) == B_OK);
		// set app info
		CHK(appFileInfo.SetVersionInfo(&testInfo2, B_SYSTEM_VERSION_KIND)
			== B_OK);
		// get
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND) == B_OK);
		CHK(info == testInfo1);
		CHK(appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND) == B_OK);
		CHK(info == testInfo2);
		version_info testInfos1[] = { testInfo1, testInfo2 };
		CheckVersionInfoAttr(file, testInfos1);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, testInfos1);
		// reset app info
		CHK(appFileInfo.SetVersionInfo(&testInfo3, B_APP_VERSION_KIND)
			== B_OK);
		// get
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND) == B_OK);
		CHK(info == testInfo3);
		CHK(appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND) == B_OK);
		CHK(info == testInfo2);
		version_info testInfos2[] = { testInfo3, testInfo2 };
		CheckVersionInfoAttr(file, testInfos2);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, testInfos2);
		// reset system info
		CHK(appFileInfo.SetVersionInfo(&testInfo4, B_SYSTEM_VERSION_KIND)
			== B_OK);
		// get
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND) == B_OK);
		CHK(info == testInfo3);
		CHK(appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND) == B_OK);
		CHK(info == testInfo4);
		version_info testInfos3[] = { testInfo3, testInfo4 };
		CheckVersionInfoAttr(file, testInfos3);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, testInfos3);
	}
	// * NULL info => B_BAD_VALUE
// R5: crashes when passing a NULL version_info
#ifndef TEST_R5
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(equals(appFileInfo.GetVersionInfo(NULL, B_APP_VERSION_KIND),
				   B_BAD_ADDRESS, B_BAD_VALUE));
		CHK(equals(appFileInfo.GetVersionInfo(NULL, B_SYSTEM_VERSION_KIND),
				   B_BAD_ADDRESS, B_BAD_VALUE));
	}
#endif

	// status_t SetVersionInfo(const version_info *info, version_kind kind)
	// * NULL info => unsets both infos, B_OK
// R5: crashes when passing a NULL version_info
#ifndef TEST_R5
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// unset app info
		CHK(appFileInfo.SetVersionInfo(NULL, B_APP_VERSION_KIND)
			== B_OK);
		// try to get
		version_info info;
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND)
			== B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND)
			== B_ENTRY_NOT_FOUND);
		// check
		CheckNoAttr(file, kVersionInfoAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kVersionInfoAttribute);
		// reset the infos
		CHK(appFileInfo.SetVersionInfo(&testInfo1, B_APP_VERSION_KIND)
			== B_OK);
		CHK(appFileInfo.SetVersionInfo(&testInfo1, B_SYSTEM_VERSION_KIND)
			== B_OK);
		// unset system info
		CHK(appFileInfo.SetVersionInfo(NULL, B_SYSTEM_VERSION_KIND) == B_OK);
		// try to get
		CHK(appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND)
			== B_ENTRY_NOT_FOUND);
		CHK(appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND)
			== B_ENTRY_NOT_FOUND);
		// check
		CheckNoAttr(file, kVersionInfoAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kVersionInfoAttribute);
	}
#endif
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetVersionInfo(&testInfo1, B_APP_VERSION_KIND)
			== B_NO_INIT);
		CHK(appFileInfo.GetVersionInfo(&testInfo1, B_SYSTEM_VERSION_KIND)
			== B_NO_INIT);
	}
}

// IconForTypeTest
void
AppFileInfoTest::IconForTypeTest()
{
	// status_t GetIconForType(const char *type, BBitmap *icon,
	//						   icon_size which) const
	// * NULL icon => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.GetIconForType(testType1, NULL, B_MINI_ICON)
			== B_BAD_VALUE);
		CHK(appFileInfo.GetIconForType(testType1, NULL, B_LARGE_ICON)
			== B_BAD_VALUE);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON)
			== B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_NO_INIT);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON)
			== B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_BAD_VALUE);
	}
	// * has no icon => B_ENTRY_NOT_FOUND
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON)
			== B_ENTRY_NOT_FOUND);
	}
	// * set, get, reset, get
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// mini
		// set
		CHK(appFileInfo.SetIconForType(testType1, fIconM1, B_MINI_ICON)
			== B_OK);
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM1, &icon));
		CheckIconForTypeAttr(file, testType1, fIconM1);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType1, fIconM1);
		// reset
		CHK(appFileInfo.SetIconForType(testType1, fIconM2, B_MINI_ICON)
			== B_OK);
		// get
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon2, B_MINI_ICON)
			== B_OK);
		CHK(icon_equal(fIconM2, &icon2));
		CheckIconForTypeAttr(file, testType1, fIconM2);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType1, fIconM2);
		// large
		// set
		CHK(appFileInfo.SetIconForType(testType1, fIconL1, B_LARGE_ICON)
			== B_OK);
		// get
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon3, B_LARGE_ICON)
			== B_OK);
		CHK(icon_equal(fIconL1, &icon3));
		CheckIconForTypeAttr(file, testType1, fIconL1);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType1, fIconL1);
		// reset
		CHK(appFileInfo.SetIconForType(testType1, fIconL2, B_LARGE_ICON)
			== B_OK);
		// get
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon4, B_LARGE_ICON)
			== B_OK);
		CHK(icon_equal(fIconL2, &icon4));
		CheckIconForTypeAttr(file, testType1, fIconL2);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType1, fIconL2);
	}
	// * bitmap color_space != B_CMAP8 => B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// mini
		BBitmap icon(BRect(0, 0, 15, 15), B_RGB32);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON) == B_OK);
		BBitmap icon2(BRect(0, 0, 15, 15), B_RGB32);
		// SetBits() can be used, since there's no row padding for 16x16.
		icon2.SetBits(fIconM2->Bits(), fIconM2->BitsLength(), 0, B_CMAP8);
		CHK(icon_equal(&icon, &icon2));
		// large
// R5: Crashes for some weird reason in GetIconForType().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CHK(appFileInfo.GetIconForType(testType1, &icon3, B_LARGE_ICON)
			== B_OK);
		BBitmap icon4(BRect(0, 0, 31, 31), B_RGB32);
		// SetBits() can be used, since there's no row padding for 32x32.
		icon4.SetBits(fIconL2->Bits(), fIconL2->BitsLength(), 0, B_CMAP8);
		CHK(icon_equal(&icon3, &icon4));
#endif
	}

	// status_t SetIconForType(const char *type, const BBitmap *icon,
	//						   icon_size which)
	// * NULL icon => unsets icon, B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// mini
		// set
		CHK(appFileInfo.SetIconForType(testType1, NULL, B_MINI_ICON) == B_OK);
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON)
			== B_ENTRY_NOT_FOUND);
		CheckNoIconForTypeAttr(file, testType1, B_MINI_ICON);
		SyncResources(appFileInfo);
		CheckNoIconForTypeResource(file, testType1, B_MINI_ICON);
		// large
		// set
		CHK(appFileInfo.SetIconForType(testType1, NULL, B_LARGE_ICON) == B_OK);
		// get
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_ENTRY_NOT_FOUND);
		CheckNoIconForTypeAttr(file, testType1, B_LARGE_ICON);
		SyncResources(appFileInfo);
		CheckNoIconForTypeResource(file, testType1, B_LARGE_ICON);
	}
	// * uninitialized => B_NO_INIT
	NextSubTest();
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.SetIconForType(testType1, &icon, B_MINI_ICON)
			== B_NO_INIT);
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.SetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_NO_INIT);
	}
	// * icon dimensions != icon size => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.SetIconForType(testType1, &icon, B_MINI_ICON)
			== B_BAD_VALUE);
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.SetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_BAD_VALUE);
	}
	// * NULL type => set standard icon
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetIconForType(NULL, fIconM1, B_MINI_ICON) == B_OK);
		CHK(appFileInfo.SetIconForType(NULL, fIconL1, B_LARGE_ICON) == B_OK);
		// get mini icon via GetIcon()
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM1, &icon));
		CheckIconAttr(file, fIconM1);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM1);
		// get large via GetIcon()
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIcon(&icon2, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL1, &icon2));
		CheckIconAttr(file, fIconL1);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL1);
		// get mini icon via GetIconForType()
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(NULL, &icon3, B_MINI_ICON) == B_OK);
		CHK(icon_equal(fIconM1, &icon3));
		CheckIconAttr(file, fIconM1);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM1);
		// get large via GetIconForType()
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(NULL, &icon4, B_LARGE_ICON) == B_OK);
		CHK(icon_equal(fIconL1, &icon4));
		CheckIconAttr(file, fIconL1);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL1);
	}
	// * set icons for two types
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// set icons for both types
		CHK(appFileInfo.SetIconForType(testType1, fIconM1, B_MINI_ICON)
			== B_OK);
		CHK(appFileInfo.SetIconForType(testType1, fIconL1, B_LARGE_ICON)
			== B_OK);
		CHK(appFileInfo.SetIconForType(testType2, fIconM2, B_MINI_ICON)
			== B_OK);
		CHK(appFileInfo.SetIconForType(testType2, fIconL2, B_LARGE_ICON)
			== B_OK);
		// check first type
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon, B_MINI_ICON)
			== B_OK);
		CHK(icon_equal(fIconM1, &icon));
		CheckIconForTypeAttr(file, testType1, fIconM1);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType1, fIconM1);
		// get large
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_OK);
		CHK(icon_equal(fIconL1, &icon2));
		CheckIconForTypeAttr(file, testType1, fIconL1);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType1, fIconL1);
		// check second type
		// get mini
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType2, &icon3, B_MINI_ICON)
			== B_OK);
		CHK(icon_equal(fIconM2, &icon3));
		CheckIconForTypeAttr(file, testType2, fIconM2);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType2, fIconM2,
								 kMiniIconForTypeResourceID + 1);
		// get large
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(testType2, &icon4, B_LARGE_ICON)
			== B_OK);
		CHK(icon_equal(fIconL2, &icon4));
		CheckIconForTypeAttr(file, testType2, fIconL2);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, testType2, fIconL2,
								 kLargeIconForTypeResourceID + 1);
		// unset both icons
		CHK(appFileInfo.SetIconForType(testType1, NULL, B_MINI_ICON) == B_OK);
		CHK(appFileInfo.SetIconForType(testType1, NULL, B_LARGE_ICON) == B_OK);
		CHK(appFileInfo.SetIconForType(testType2, NULL, B_MINI_ICON) == B_OK);
		CHK(appFileInfo.SetIconForType(testType2, NULL, B_LARGE_ICON) == B_OK);
	}
	// * invalid type => B_OK
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetIconForType(invalidTestType, fIconM1, B_MINI_ICON)
			== B_BAD_VALUE);
		CHK(appFileInfo.SetIconForType(invalidTestType, fIconL1, B_LARGE_ICON)
			== B_BAD_VALUE);
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(appFileInfo.GetIconForType(invalidTestType, &icon, B_MINI_ICON)
			== B_BAD_VALUE);
		CheckNoIconForTypeAttr(file, invalidTestType, B_LARGE_ICON);
		CheckNoIconForTypeResource(file, invalidTestType, B_LARGE_ICON);
		// get large
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(appFileInfo.GetIconForType(invalidTestType, &icon3, B_LARGE_ICON)
			== B_BAD_VALUE);
		CheckNoIconForTypeAttr(file, invalidTestType, B_MINI_ICON);
		CheckNoIconForTypeResource(file, invalidTestType, B_MINI_ICON);
	}
	// * too long type => B_BAD_VALUE
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetIconForType(tooLongTestType, fIconM1, B_MINI_ICON)
			== B_BAD_VALUE);
		CHK(appFileInfo.SetIconForType(tooLongTestType, fIconL1, B_LARGE_ICON)
			== B_BAD_VALUE);
	}
	// * file has app signature, set an icon => the icon is also set on the
	//   MIME type
	NextSubTest();
	{
		BFile file(testFile1, B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		// uninstalled signature
		BMimeType mimeType(testAppSignature1);
		CHK(mimeType.IsInstalled() == false);
		CHK(appFileInfo.SetSignature(testAppSignature1) == B_OK);
		CHK(appFileInfo.SetIconForType(testType1, fIconM1, B_MINI_ICON)
			== B_OK);
		CHK(appFileInfo.SetIconForType(testType1, fIconL1, B_LARGE_ICON)
			== B_OK);
		CHK(mimeType.IsInstalled() == true);
		CHK(BMimeType(testType1).IsInstalled() == false);
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(mimeType.GetIconForType(testType1, &icon, B_MINI_ICON)
			== B_OK);
		CHK(icon_equal(fIconM1, &icon));
		// get large
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(mimeType.GetIconForType(testType1, &icon2, B_LARGE_ICON)
			== B_OK);
		CHK(icon_equal(fIconL1, &icon2));
		// installed signature
		CHK(appFileInfo.SetIconForType(testType1, fIconM2, B_MINI_ICON)
			== B_OK);
		CHK(appFileInfo.SetIconForType(testType1, fIconL2, B_LARGE_ICON)
			== B_OK);
		// get mini
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CHK(mimeType.GetIconForType(testType1, &icon3, B_MINI_ICON)
			== B_OK);
		CHK(icon_equal(fIconM2, &icon3));
		// get large
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CHK(mimeType.GetIconForType(testType1, &icon4, B_LARGE_ICON)
			== B_OK);
		CHK(icon_equal(fIconL2, &icon4));
	}
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
	SyncResources(appFileInfo);
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
	SyncResources(appFileInfo);
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
	SyncResources(appFileInfo);
	// get value
	Value value3;
	CHK(Getter::Get(appFileInfo, value3) == B_OK);
	CHK(value3 == testValue3);
	// check attribute/resource -- attribute should be old
	Checker::CheckAttribute(file, testValue2);
	Checker::CheckResource(file, testValue3);
}

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
	InfoLocationTester<IconForTypeValue, IconForTypeSetter, IconForTypeGetter,
					   IconForTypeChecker>(
		IconForTypeValue(fIconM1, fIconL1),
		IconForTypeValue(fIconM2, fIconL2),
		IconForTypeValue(fIconM3, fIconL3));
}

