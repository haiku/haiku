/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sstream>
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
#include <StorageDefs.h>
#include <String.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "InfoTest.h"
#include "MimeType.h"


static const char *invalidTestType	= "invalid/mime/type";
static const char *tooLongTestType	=
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
;

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
	VERSION_INFO_TYPE			= 'APPV',
};


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

class BApplication;
class BBitmap;

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

static
char*
ReadAttr(BNode &node, const char *name, type_code type, size_t &size)
{
	attr_info info;
	CPPUNIT_ASSERT_EQUAL(B_OK, node.GetAttrInfo(name, &info));
	CPPUNIT_ASSERT(info.type == type);
	char *buffer = new char[info.size];
	if (node.ReadAttr(name, type, 0, buffer, info.size) == info.size)
		size = info.size;
	else {
		delete[] buffer;
		CPPUNIT_ASSERT(false);
	}
	return buffer;
}

static
char*
ReadResource(BFile &file, const char *name, int32 id, type_code type,
			 size_t &size)
{
	BResources resources;
	CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file));
	int32 idFound;
	CPPUNIT_ASSERT(resources.GetResourceInfo(type, name, &idFound, &size) == true);
	CPPUNIT_ASSERT(idFound == id);
	const void *resourceData = resources.LoadResource(type, name, &size);
	CPPUNIT_ASSERT(resourceData != NULL);
	char *buffer = new char[size];
	memcpy(buffer, resourceData, size);
	return buffer;
}

static
void
CheckResource(BFile &file, const char *name, int32 id, type_code type,
			  const void *data, size_t dataSize)
{
	BResources resources;
	CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file));
	int32 idFound;
	size_t size;
	CPPUNIT_ASSERT(resources.GetResourceInfo(type, name, &idFound, &size) == true);
	CPPUNIT_ASSERT(idFound == id);
	CPPUNIT_ASSERT(size == dataSize);
	const void *resourceData = resources.LoadResource(type, name, &size);
	CPPUNIT_ASSERT(resourceData != NULL);
	CPPUNIT_ASSERT(size == dataSize);
	CPPUNIT_ASSERT(memcmp(resourceData, data, dataSize) == 0);
}


static
void
CheckNoResource(BFile &file, const char *name)
{
	BResources resources;
	CPPUNIT_ASSERT_EQUAL(B_OK, resources.SetTo(&file));
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
	CPPUNIT_ASSERT(!found);
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

	std::string type;
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
		InfoTest::CheckAttr(file, kTypeAttribute, B_MIME_STRING_TYPE, value.type.c_str(),
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
		InfoTest::CheckAttr(file, kSignatureAttribute, B_MIME_STRING_TYPE, value.type.c_str(),
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
		InfoTest::CheckAttr(file, kAppFlagsAttribute, APP_FLAGS_TYPE, &value.flags,
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
		BPrivate::ArrayDeleter<char> deleter(buffer);
		BMessage storedTypes;
		CPPUNIT_ASSERT_EQUAL(B_OK, storedTypes.Unflatten(buffer));
		SupportedTypesValue storedValue(storedTypes);
		CPPUNIT_ASSERT(storedValue == value);
	}

	static void CheckResource(BFile &file, const SupportedTypesValue &value)
	{
		size_t size;
		char *buffer = ReadResource(file, kSupportedTypesAttribute,
									kSupportedTypesResourceID, B_MESSAGE_TYPE,
									size);
		BPrivate::ArrayDeleter<char> deleter(buffer);
		BMessage storedTypes;
		CPPUNIT_ASSERT_EQUAL(B_OK, storedTypes.Unflatten(buffer));
		SupportedTypesValue storedValue(storedTypes);
		CPPUNIT_ASSERT(storedValue == value);
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
		return (InfoTest::IconEqual(&mini, &value.mini)
				&& InfoTest::IconEqual(&large, &value.large));
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
		InfoTest::CheckAttr(file, kMiniIconAttribute, B_MINI_ICON_TYPE, value.mini.Bits(),
				  value.mini.BitsLength());
		InfoTest::CheckAttr(file, kLargeIconAttribute, B_LARGE_ICON_TYPE,
				  value.large.Bits(), value.large.BitsLength());
	}

	static void CheckResource(BFile &file, const IconValue &value)
	{
		::CheckResource(file, kMiniIconAttribute, kMiniIconResourceID,
						B_MINI_ICON_TYPE, value.mini.Bits(),
						value.mini.BitsLength());
		::CheckResource(file, kLargeIconAttribute, kLargeIconResourceID,
						B_LARGE_ICON_TYPE, value.large.Bits(),
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
		InfoTest::CheckAttr(file, kVersionInfoAttribute, VERSION_INFO_TYPE,
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
	IconForTypeValue() : type("application/x-vnd.obos.app-file-info-test1") {}
	IconForTypeValue(const BBitmap *mini, const BBitmap *large)
		: IconValue(mini, large), type("application/x-vnd.obos.app-file-info-test1") {}

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
		std::string attrNameM(kMiniIconForTypeAttribute);
		attrNameM += value.type;
		InfoTest::CheckAttr(file, attrNameM.c_str(), B_MINI_ICON_TYPE, value.mini.Bits(),
				  value.mini.BitsLength());
		std::string attrNameL(kLargeIconForTypeAttribute);
		attrNameL += value.type;
		InfoTest::CheckAttr(file, attrNameL.c_str(), B_LARGE_ICON_TYPE,
				  value.large.Bits(), value.large.BitsLength());
	}

	static void CheckResource(BFile &file, const IconForTypeValue &value)
	{
		std::string attrNameM(kMiniIconForTypeAttribute);
		attrNameM += value.type;
		::CheckResource(file, attrNameM.c_str(), kMiniIconForTypeResourceID,
						B_MINI_ICON_TYPE, value.mini.Bits(),
						value.mini.BitsLength());
		std::string attrNameL(kLargeIconForTypeAttribute);
		attrNameL += value.type;
		::CheckResource(file, attrNameL.c_str(), kLargeIconForTypeResourceID,
						B_LARGE_ICON_TYPE, value.large.Bits(),
						value.large.BitsLength());
	}
};



static
void
CheckTypeResource(BFile &file, const char *data)
{
	CheckResource(file, kTypeAttribute, kTypeResourceID, B_MIME_STRING_TYPE,
				  data, strlen(data) + 1);
}

static
void
CheckSignatureAttr(BNode &node, const char *data)
{
	InfoTest::CheckAttr(node, kSignatureAttribute, B_MIME_STRING_TYPE, data,
			  strlen(data) + 1);
}

static
void
CheckSignatureResource(BFile &file, const char *data)
{
	CheckResource(file, kSignatureAttribute, kSignatureResourceID,
				  B_MIME_STRING_TYPE, data, strlen(data) + 1);
}

static
void
CheckAppFlagsAttr(BNode &node, uint32 flags)
{
	InfoTest::CheckAttr(node, kAppFlagsAttribute, APP_FLAGS_TYPE, &flags, sizeof(flags));
}

static
void
CheckAppFlagsResource(BFile &file, uint32 flags)
{
	CheckResource(file, kAppFlagsAttribute, kAppFlagsResourceID,
				  APP_FLAGS_TYPE, &flags, sizeof(flags));
}

static
void
CheckSupportedTypesAttr(BNode &node, const BMessage *data)
{
	SupportedTypesChecker::CheckAttribute(node, SupportedTypesValue(*data));
}

static
void
CheckSupportedTypesResource(BFile &file, const BMessage *data)
{
	SupportedTypesChecker::CheckResource(file, SupportedTypesValue(*data));
}

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
			type = B_MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconAttribute;
			resourceID = kLargeIconResourceID;
			type = B_LARGE_ICON_TYPE;
			break;
		default:
			CPPUNIT_ASSERT(false);
			break;
	}
	CheckResource(file, attribute, resourceID, type, data->Bits(),
				  data->BitsLength());
}

static
void
CheckVersionInfoAttr(BNode &node, version_info *data)
{
	InfoTest::CheckAttr(node, kVersionInfoAttribute, VERSION_INFO_TYPE, data,
			  2 * sizeof(version_info));
}

static
void
CheckVersionInfoResource(BFile &file, version_info *data)
{
	CheckResource(file, kVersionInfoAttribute, kVersionInfoResourceID,
				  VERSION_INFO_TYPE, data, 2 * sizeof(version_info));
}

static
void
CheckIconForTypeAttr(BNode &node, const char *type, BBitmap *data)
{
	std::string attribute;
	uint32 iconType = 0;
	switch (data->Bounds().IntegerWidth()) {
		case 15:
			attribute = kMiniIconForTypeAttribute;
			iconType = B_MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconForTypeAttribute;
			iconType = B_LARGE_ICON_TYPE;
			break;
		default:
			CPPUNIT_ASSERT(false);
			break;
	}
	attribute += type;
	InfoTest::CheckAttr(node, attribute.c_str(), iconType, data->Bits(),
			  data->BitsLength());
}

static
void
CheckIconForTypeResource(BFile &file, const char *type, BBitmap *data,
						 int32 resourceID = -1)
{
	std::string attribute;
	uint32 iconType = 0;
	switch (data->Bounds().IntegerWidth()) {
		case 15:
			attribute = kMiniIconForTypeAttribute;
			if (resourceID < 0)
				resourceID = kMiniIconForTypeResourceID;
			iconType = B_MINI_ICON_TYPE;
			break;
		case 31:
			attribute = kLargeIconForTypeAttribute;
			if (resourceID < 0)
				resourceID = kLargeIconForTypeResourceID;
			iconType = B_LARGE_ICON_TYPE;
			break;
		default:
			CPPUNIT_ASSERT(false);
			break;
	}
	attribute += type;
	CheckResource(file, attribute.c_str(), resourceID, iconType, data->Bits(),
				  data->BitsLength());
}

static
void
CheckNoIconForTypeAttr(BNode &node, const char *type, icon_size iconSize)
{
	std::string attribute;
	switch (iconSize) {
		case B_MINI_ICON:
			attribute = kMiniIconForTypeAttribute;
			break;
		case B_LARGE_ICON:
			attribute = kLargeIconForTypeAttribute;
			break;
		default:
			CPPUNIT_ASSERT(false);
			break;
	}
	attribute += type;
	InfoTest::CheckNoAttr(node, attribute.c_str());
}

static
void
CheckNoIconForTypeResource(BFile &file, const char *type, icon_size iconSize)
{
	std::string attribute;
	switch (iconSize) {
		case B_MINI_ICON:
			attribute = kMiniIconForTypeAttribute;
			break;
		case B_LARGE_ICON:
			attribute = kLargeIconForTypeAttribute;
			break;
		default:
			CPPUNIT_ASSERT(false);
			break;
	}
	attribute += type;
	CheckNoResource(file, attribute.c_str());
}

static
bool
IsSupportingApp(const char *type, const char *signature)
{
	BMessage apps;
//	CPPUNIT_ASSERT_EQUAL(B_OK, BMimeType(type).GetSupportingApps(&apps));
	BMimeType(type).GetSupportingApps(&apps);
	bool found = false;
	BString app;
	for (int32 i = 0; apps.FindString("applications", i, &app) == B_OK; i++) {
		if (app == signature) {
			found = true;
			break;
		}
	}
	return found;
}


template<typename Value, typename Setter, typename Getter, typename Checker>
static void
InfoLocationTester(BFile &file, const Value &testValue1, const Value &testValue2,
				   const Value &testValue3)
{
	BAppFileInfo appFileInfo;
	CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
	CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.InitCheck());
	CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
	CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
	// set value
	CPPUNIT_ASSERT_EQUAL(B_OK, Setter::Set(appFileInfo, testValue1));
	CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
	CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
	// force resources synchronization
	SyncResources(appFileInfo);
	// get value
	Value value;
	CPPUNIT_ASSERT_EQUAL(B_OK, Getter::Get(appFileInfo, value));
	CPPUNIT_ASSERT(value == testValue1);
	// check attribute/resource
	Checker::CheckAttribute(file, testValue1);
	Checker::CheckResource(file, testValue1);
	// set on B_USE_ATTRIBUTES
	appFileInfo.SetInfoLocation(B_USE_ATTRIBUTES);
	CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
	CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == false);
	// set value
	CPPUNIT_ASSERT_EQUAL(B_OK, Setter::Set(appFileInfo, testValue2));
	// force resources synchronization
	SyncResources(appFileInfo);
	// get value
	Value value1;
	CPPUNIT_ASSERT_EQUAL(B_OK, Getter::Get(appFileInfo, value1));
	CPPUNIT_ASSERT(value1 == testValue2);
	// check attribute/resource -- resource should be old
	Checker::CheckAttribute(file, testValue2);
	Checker::CheckResource(file, testValue1);
	// set on B_USE_BOTH_LOCATIONS
	appFileInfo.SetInfoLocation(B_USE_BOTH_LOCATIONS);
	CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
	CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
	// get value -- the attribute is preferred
	Value value2;
	CPPUNIT_ASSERT_EQUAL(B_OK, Getter::Get(appFileInfo, value2));
	CPPUNIT_ASSERT(value2 == testValue2);
	// set on B_USE_RESOURCES
	appFileInfo.SetInfoLocation(B_USE_RESOURCES);
	CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == false);
	CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
	// set value
	CPPUNIT_ASSERT_EQUAL(B_OK, Setter::Set(appFileInfo, testValue3));
	// force resources synchronization
	SyncResources(appFileInfo);
	// get value
	Value value3;
	CPPUNIT_ASSERT_EQUAL(B_OK, Getter::Get(appFileInfo, value3));
	CPPUNIT_ASSERT(value3 == testValue3);
	Checker::CheckAttribute(file, testValue2);
	Checker::CheckResource(file, testValue3);
}


class AppFileInfoTest : public InfoTest {
	CPPUNIT_TEST_SUITE(AppFileInfoTest);

	CPPUNIT_TEST(Default_Constructor_ReturnsNoInit);
	CPPUNIT_TEST(NullFile_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(InvalidFile_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(ValidFile_Constructor_ReturnsOk);

	CPPUNIT_TEST(NullFile_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(InvalidFile_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(ValidFile_SetTo_ReturnsOk);
	CPPUNIT_TEST(Reinitialize_SetTo_ReturnsExpected);

	CPPUNIT_TEST(NullType_GetType_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_GetType_ReturnsNoInit);
	CPPUNIT_TEST(NoType_GetType_ReturnsEntryNotFound);

	CPPUNIT_TEST(NullType_SetType_UnsetsTypeReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetType_ReturnsNoInit);
	CPPUNIT_TEST(Valid_SetType_ReturnsOk);
	CPPUNIT_TEST(InvalidMime_SetType_ReturnsOk);
	CPPUNIT_TEST(TooLongMime_SetType_ReturnsBadValue);

	CPPUNIT_TEST(NullSignature_GetSignature_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_GetSignature_ReturnsNoInit);
	CPPUNIT_TEST(NoSignature_GetSignature_ReturnsEntryNotFound);
	CPPUNIT_TEST(Valid_GetSignature_ReturnsOk);

	CPPUNIT_TEST(NullSignature_SetSignature_UnsetsSignatureReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetSignature_ReturnsNoInit);
	CPPUNIT_TEST(Valid_SetSignature_ReturnsOk);
	CPPUNIT_TEST(InvalidMime_SetSignature_ReturnsOk);
	CPPUNIT_TEST(TooLongMime_SetSignature_ReturnsBadValue);

	CPPUNIT_TEST(NullFlags_GetAppFlags_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_GetAppFlags_ReturnsNoInit);
	CPPUNIT_TEST(NoFlags_GetAppFlags_ReturnsEntryNotFound);

	CPPUNIT_TEST(Uninitialized_SetAppFlags_ReturnsNoInit);
	CPPUNIT_TEST(Valid_SetAppFlags_ReturnsOk);

	CPPUNIT_TEST(NullMessage_GetSupportedTypes_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_GetSupportedTypes_ReturnsNoInit);
	CPPUNIT_TEST(NoSupportedTypes_GetSupportedTypes_ReturnsEntryNotFound);
	CPPUNIT_TEST(PartiallyFilledMessage_GetSupportedTypes_ClearsInvalidFieldsReturnsOk);

	CPPUNIT_TEST(NullMessage_SetSupportedTypes_UnsetsTypesReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetSupportedTypes_ReturnsNoInit);
	CPPUNIT_TEST(Valid_SetSupportedTypes_ReturnsOk);
	CPPUNIT_TEST(SyncAll_SetSupportedTypes_ReturnsExpected);
	CPPUNIT_TEST(NoSyncAll_SetSupportedTypes_ReturnsExpected);
	CPPUNIT_TEST(InvalidMime_SetSupportedTypes_ReturnsBadValue);
	CPPUNIT_TEST(SignatureNotInstalled_SetSupportedTypesSyncAll_ReturnsExpected);
	CPPUNIT_TEST(SignatureNotInstalled_SetSupportedTypes_ReturnsExpected);
	CPPUNIT_TEST(NoAppSignature_SetSupportedTypesSyncAll_ReturnsExpected);
	CPPUNIT_TEST(NoAppSignature_SetSupportedTypes_ReturnsExpected);
	CPPUNIT_TEST(RemoveFileCreateFile_SetSupportedTypes_ReturnsExpected);

	CPPUNIT_TEST(Valid_IsSupportedType_ReturnsExpected);
	CPPUNIT_TEST(NullType_IsSupportedType_ReturnsFalse);
	CPPUNIT_TEST(OctetStreamSupported_IsSupportedTypeImageGif_ReturnsTrue);

	CPPUNIT_TEST(Valid_Supports_ReturnsExpected);
	CPPUNIT_TEST(NullType_Supports_ReturnsFalse);
	CPPUNIT_TEST(OctetStreamSupported_SupportsImageGif_ReturnsFalse);

	CPPUNIT_TEST(NullIcon_GetIcon_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_GetIcon_ReturnsNoInit);
	CPPUNIT_TEST(WrongIconDimensions_GetIcon_ReturnsBadValue);
	CPPUNIT_TEST(NoIcon_GetIcon_ReturnsEntryNotFound);
	CPPUNIT_TEST(SetGetResetGet_SetIcon_ReturnsExpected);
	CPPUNIT_TEST(BitmapColorSpaceNotCMAP8_GetIcon_ReturnsOk);

	CPPUNIT_TEST(NullIcon_SetIcon_UnsetsIconReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetIcon_ReturnsNoInit);
	CPPUNIT_TEST(WrongIconDimensions_SetIcon_ReturnsBadValue);
	CPPUNIT_TEST(FileHasAppSignature_SetIcon_IconAlsoSetOnMimeType);

	CPPUNIT_TEST(Uninitialized_GetVersionInfo_ReturnsNoInit);
	CPPUNIT_TEST(NullInfo_GetVersionInfo_ReturnsBadValue);
	CPPUNIT_TEST(NoVersionInfo_GetVersionInfo_ReturnsEntryNotFound);

	CPPUNIT_TEST(Uninitialized_SetVersionInfo_ReturnsNoInit);
	CPPUNIT_TEST(Valid_SetVersionInfo_ReturnsOk);
	CPPUNIT_TEST(Null_SetVersionInfo_UnsetsBothKindsReturnsOk);
	CPPUNIT_TEST(SetGetResetGetAppKind_SetVersionInfo_ReturnsExpected);
	CPPUNIT_TEST(SetGetResetGetSystemKind_SetVersionInfo_ReturnsExpected);

	CPPUNIT_TEST(Uninitialized_GetIconForType_ReturnsNoInit);
	CPPUNIT_TEST(NullIcon_GetIconForType_ReturnsBadValue);
	CPPUNIT_TEST(WrongIconDimensions_GetIconForType_ReturnsBadValue);
	CPPUNIT_TEST(NoIcon_GetIconForType_ReturnsEntryNotFound);
	CPPUNIT_TEST(BitmapColorSpaceNotCMAP8_GetIconForType_ReturnsOk);

	CPPUNIT_TEST(Valid_SetIconForType_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetIconForType_ReturnsNoInit);
	CPPUNIT_TEST(WrongIconDimensions_SetIconForType_ReturnsBadValue);
	CPPUNIT_TEST(SetGetResetGet_SetIconForTypeMiniIcon_ReturnsExpected);
	CPPUNIT_TEST(SetGetResetGet_SetIconForTypeLargeIcon_ReturnsExpected);
	CPPUNIT_TEST(NullMiniIcon_SetIconForType_UnsetsIconReturnsOk);
	CPPUNIT_TEST(NullLargeIcon_SetIconForType_UnsetsIconReturnsOk);
	CPPUNIT_TEST(NullType_SetIconForType_SetsStandardIcon);
	CPPUNIT_TEST(TwoTypesSetGet_SetIconForType_ReturnsExpected);
	CPPUNIT_TEST(InvalidType_SetIconForType_ReturnsBadValue);
	CPPUNIT_TEST(TooLongType_SetIconForType_ReturnsBadValue);
	CPPUNIT_TEST(FileHasAppSignature_SetIconForType_IconAlsoSetOnMimeType);
	CPPUNIT_TEST(SetGetResetGet_SetIconForType_ReturnsExpected);

#if __GNUC__ > 2
	CPPUNIT_TEST(ValidType_SetInfoLocation_ReturnsExpected);
	CPPUNIT_TEST(ValidSignature_SetInfoLocation_ReturnsExpected);
	CPPUNIT_TEST(ValidAppFlags_SetInfoLocation_ReturnsExpected);
	CPPUNIT_TEST(ValidSupportedTypes_SetInfoLocation_ReturnsExpected);
	CPPUNIT_TEST(ValidIcon_SetInfoLocation_ReturnsExpected);
	CPPUNIT_TEST(ValidVersionInfo_SetInfoLocation_ReturnsExpected);
	CPPUNIT_TEST(ValidIconForType_SetInfoLocation_ReturnsExpected);
#endif

	CPPUNIT_TEST_SUITE_END();

	BMimeType		fMimeTestType[4];
	version_info	fTestInfo[4];
	BMessage		fTestTypes[2];

	BApplication	*fApplication;
	const bool		fHasInitialLocation;


public:
	AppFileInfoTest()
	:
	InfoTest("application/x-vnd.obos.app-file-info-test"),
#ifndef TEST_R5
	fHasInitialLocation(true)
#else
	fHasInitialLocation(false)
#endif
	{
		for (int i = 0; i < 4; i++) {
			fMimeTestType[i].SetTo(fMimeTypeString[i].c_str());
			uint32 fill = static_cast<uint32>(i);
			fTestInfo[i] = (version_info){ fill, fill, fill, fill, fill, "short1", "long1" };
			fTestInfo[i].short_info[5] += i;
			fTestInfo[i].long_info[4] += i;
			fTestTypes[i / 2].AddString("types", fMimeTypeString[i].c_str());
		}
	}

	~AppFileInfoTest() {
	}

	void setUp()
	{
		// app has to be created here instead of constructor, to avoid
		// creating two application objects
		fApplication = new BApplication(kMimeTypeBase);

		InfoTest::setUp();
	}

	void tearDown()
	{
		InfoTest::tearDown();

		delete fApplication;
		fApplication = NULL;
	}

	void Default_Constructor_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == fHasInitialLocation);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == fHasInitialLocation);
	}

	void NullFile_Constructor_ReturnsBadValue()
	{
		BAppFileInfo appFileInfo(NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == fHasInitialLocation);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == fHasInitialLocation);
	}

	void InvalidFile_Constructor_ReturnsBadValue()
	{
		BFile file;
		BAppFileInfo appFileInfo(&file);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == fHasInitialLocation);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == fHasInitialLocation);
	}

	void ValidFile_Constructor_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo(&file);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
	}

	void NullFile_SetTo_ReturnsBadValue()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetTo(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == fHasInitialLocation);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == fHasInitialLocation);
	}

	void InvalidFile_SetTo_ReturnsBadValue()
	{
		BFile file;
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == fHasInitialLocation);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == fHasInitialLocation);
	}

	void ValidFile_SetTo_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);

		off_t size;
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 0);
		CheckNoAttr(file, kTypeAttribute);
		CheckNoAttr(file, kMiniIconAttribute);
		CheckNoAttr(file, kLargeIconAttribute);
	}

	void Reinitialize_SetTo_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.InitCheck());
		off_t size;

		// reinit with NULL file
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetTo(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 0);
		CheckNoAttr(file, kTypeAttribute);
		CheckNoAttr(file, kMiniIconAttribute);
		CheckNoAttr(file, kLargeIconAttribute);

		// reinit with valid file
		BFile file2(fTestFilePaths[1].c_str(), B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file2));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.InitCheck());
		CPPUNIT_ASSERT(appFileInfo.IsUsingAttributes() == true);
		CPPUNIT_ASSERT(appFileInfo.IsUsingResources() == true);
	}	// Type

	void NullType_GetType_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetType(NULL));
	}

	void Uninitialized_GetType_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetType(buffer));
	}

	void NoType_GetType_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetType(buffer));
	}

	void NullType_SetType_UnsetsTypeReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetType(fMimeTypeString[0].c_str()));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetType(NULL));
		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetType(buffer));
		CheckNoAttr(file, kTypeAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kTypeAttribute);
	}

	void Uninitialized_SetType_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetType(fMimeTypeString[0].c_str()));
	}

	void Valid_SetType_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetType(fMimeTypeString[0].c_str()));

		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetType(buffer));
		CPPUNIT_ASSERT(strcmp(fMimeTypeString[0].c_str(), buffer) == 0);
		CheckTypeAttr(file, fMimeTypeString[0].c_str());
		SyncResources(appFileInfo);
		CheckTypeResource(file, fMimeTypeString[0].c_str());

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetType(fMimeTypeString[1].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetType(buffer));
		CPPUNIT_ASSERT(strcmp(fMimeTypeString[1].c_str(), buffer) == 0);
		CheckTypeAttr(file, fMimeTypeString[1].c_str());
		SyncResources(appFileInfo);
		CheckTypeResource(file, fMimeTypeString[1].c_str());
	}

	void InvalidMime_SetType_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetType(invalidTestType));

		char type[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetType(type));
		CPPUNIT_ASSERT(strcmp(invalidTestType, type) == 0);
		CheckTypeAttr(file, invalidTestType);
		SyncResources(appFileInfo);
		CheckTypeResource(file, invalidTestType);
	}

	void TooLongMime_SetType_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		// remove attr first
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetType(NULL));
		// try to set the too long type
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetType(tooLongTestType));
		CheckNoAttr(file, kTypeAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kTypeAttribute);
	}

	void NullSignature_GetSignature_ReturnsBadValue()
	{

		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetSignature(NULL));
	}

	void Uninitialized_GetSignature_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetSignature(buffer));
	}

	void NoSignature_GetSignature_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSignature(buffer));
	}

	void Valid_GetSignature_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSignature(buffer));
		CPPUNIT_ASSERT(strcmp(fMimeTypeAppString[0].c_str(), buffer) == 0);
	}

	void NullSignature_SetSignature_UnsetsSignatureReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(NULL));
		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSignature(buffer));
		CheckNoAttr(file, kSignatureAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSignatureAttribute);
	}

	void Uninitialized_SetSignature_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));
	}

	void Valid_SetSignature_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT(BMimeType(fMimeTypeAppString[0].c_str()).IsInstalled() == false);

		char buffer[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSignature(buffer));
		CPPUNIT_ASSERT(strcmp(fMimeTypeAppString[0].c_str(), buffer) == 0);
		CheckSignatureAttr(file, fMimeTypeAppString[0].c_str());
		SyncResources(appFileInfo);
		CheckSignatureResource(file, fMimeTypeAppString[0].c_str());

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[1].c_str()));
		CPPUNIT_ASSERT(BMimeType(fMimeTypeAppString[1].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSignature(buffer));
		CPPUNIT_ASSERT(strcmp(fMimeTypeAppString[1].c_str(), buffer) == 0);
		CheckSignatureAttr(file, fMimeTypeAppString[1].c_str());
		SyncResources(appFileInfo);
		CheckSignatureResource(file, fMimeTypeAppString[1].c_str());
	}

	void InvalidMime_SetSignature_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(invalidTestType));
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSignature(signature));
		CPPUNIT_ASSERT(strcmp(invalidTestType, signature) == 0);
		CheckSignatureAttr(file, invalidTestType);
		SyncResources(appFileInfo);
		CheckSignatureResource(file, invalidTestType);
	}

	void TooLongMime_SetSignature_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		// remove attr first
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(NULL));
		// try to set the too long signature
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetSignature(tooLongTestType));
		CheckNoAttr(file, kSignatureAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSignatureAttribute);
	}

	void NullFlags_GetAppFlags_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetAppFlags(NULL));
	}

	void Uninitialized_GetAppFlags_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		uint32 flags;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetAppFlags(&flags));
	}

	void NoFlags_GetAppFlags_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		uint32 flags;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetAppFlags(&flags));
	}

	void Uninitialized_SetAppFlags_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetAppFlags(B_SINGLE_LAUNCH));
	}

	void Valid_SetAppFlags_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		uint32 flags = B_SINGLE_LAUNCH | B_BACKGROUND_APP;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetAppFlags(flags));

		uint32 getFlags;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetAppFlags(&getFlags));
		CPPUNIT_ASSERT(flags == getFlags);
		CheckAppFlagsAttr(file, flags);
		SyncResources(appFileInfo);
		CheckAppFlagsResource(file, flags);

		flags = B_MULTIPLE_LAUNCH | B_ARGV_ONLY;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetAppFlags(flags));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetAppFlags(&getFlags));
		CPPUNIT_ASSERT(flags == getFlags);
		CheckAppFlagsAttr(file, flags);
		SyncResources(appFileInfo);
		CheckAppFlagsResource(file, flags);
	}

	void NullMessage_GetSupportedTypes_ReturnsBadValue()
	{
		// R5: crashes when passing NULL types
#ifndef TEST_R5
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetSupportedTypes(NULL));
#endif
	}

	void Uninitialized_GetSupportedTypes_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		BMessage message;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetSupportedTypes(&message));
	}

	void NoSupportedTypes_GetSupportedTypes_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BMessage message;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&message));
	}

	void PartiallyFilledMessage_GetSupportedTypes_ClearsInvalidFieldsReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BMessage types2;
		CPPUNIT_ASSERT_EQUAL(B_OK, types2.AddString("types", fMimeTypeString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, types2.AddString("dummy", "Hello"));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&types2));
		CPPUNIT_ASSERT(SupportedTypesValue(types2) == SupportedTypesValue(fTestTypes[1]));
		const char *dummy;
		CPPUNIT_ASSERT(types2.FindString("dummy", &dummy) != B_OK);
	}

	void NullMessage_SetSupportedTypes_UnsetsTypesReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(NULL));
		BMessage message;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&message));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[2].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[3].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[2]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[3]) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);

#ifndef TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(NULL, true));
		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[2].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[3].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[2]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[3]) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
#endif
	}

	void Uninitialized_SetSupportedTypes_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetSupportedTypes(&fTestTypes[0]));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetSupportedTypes(&fTestTypes[0], true));
	}

	void Valid_SetSupportedTypes_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		BMessage message;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&message));
		SupportedTypesValue value(message);
		CPPUNIT_ASSERT(value == fTestTypes[0]);
		CheckSupportedTypesAttr(file, &fTestTypes[0]);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &fTestTypes[0]);
	}

	void SyncAll_SetSupportedTypes_ReturnsExpected()
	{
		CPPUNIT_ASSERT_EQUAL(B_OK, BMimeType(fMimeTypeAppString[0].c_str()).Install());

		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0], true));

		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[0].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[1].c_str(), fMimeTypeAppString[0].c_str()) == true);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[1], true));

		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[0].c_str(), fMimeTypeAppString[0].c_str()) == false);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[1].c_str(), fMimeTypeAppString[0].c_str()) == false);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[2].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[3].c_str(), fMimeTypeAppString[0].c_str()) == true);
	}

	void NoSyncAll_SetSupportedTypes_ReturnsExpected()
	{
		CPPUNIT_ASSERT_EQUAL(B_OK, BMimeType(fMimeTypeAppString[0].c_str()).Install());

		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[0].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[1].c_str(), fMimeTypeAppString[0].c_str()) == true);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[1]));

		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[0].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[1].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[2].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[3].c_str(), fMimeTypeAppString[0].c_str()) == true);
	}

	void InvalidMime_SetSupportedTypes_ReturnsBadValue()
	{
	// R5: doesn't fail
#ifndef TEST_R5
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		BMessage invalidTestTypes;
		CPPUNIT_ASSERT_EQUAL(B_OK, invalidTestTypes.AddString("types", invalidTestType));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetSupportedTypes(&invalidTestTypes, false));

		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(invalidTestType) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
#endif
	}

	void SignatureNotInstalled_SetSupportedTypesSyncAll_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0], true));

		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[0]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == true);
		CheckSupportedTypesAttr(file, &fTestTypes[0]);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &fTestTypes[0]);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);

		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[2].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[3].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[1], true));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[0]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[2].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[3].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[2]) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[3]) == true);
		CheckSupportedTypesAttr(file, &fTestTypes[1]);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &fTestTypes[1]);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[2].c_str()).IsInstalled() == false);
// R5: returns true. In fact the last SetSupportedTypes() installed testType4
// in the database, but not testType3. Certainly a bug
#ifndef TEST_R5
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[3].c_str()).IsInstalled() == false);
#endif
	}

	void SignatureNotInstalled_SetSupportedTypes_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
// R5: SetSupportedTypes() returns B_ENTRY_NOT_FOUND, although it does not
//     fail.
#ifdef TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.SetSupportedTypes(&fTestTypes[0]));
#else
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));
#endif

		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[0]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == true);
		CheckSupportedTypesAttr(file, &fTestTypes[0]);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &fTestTypes[0]);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);

		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[2].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[3].c_str()).IsInstalled() == false);
// R5: SetSupportedTypes() returns B_ENTRY_NOT_FOUND, although it does not
//     fail.
#ifdef TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.SetSupportedTypes(&fTestTypes[1]));
#else
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[1]));
#endif

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[1]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[2].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[3].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[2]) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[3]) == true);
		CheckSupportedTypesAttr(file, &fTestTypes[1]);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &fTestTypes[1]);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[2].c_str()).IsInstalled() == false);
// R5: returns true. In fact the last SetSupportedTypes() installed testType4
// in the database, but not testType3. Certainly a bug
#ifndef TEST_R5
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[3].c_str()).IsInstalled() == false);
#endif
	}

	void NoAppSignature_SetSupportedTypesSyncAll_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.SetSupportedTypes(&fTestTypes[0], true));

		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[0]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
	}

	void NoAppSignature_SetSupportedTypes_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[0]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[1].c_str()).IsInstalled() == false);
	}

	void RemoveFileCreateFile_SetSupportedTypes_ReturnsExpected()
	{
		CPPUNIT_ASSERT_EQUAL(B_OK, BMimeType(fMimeTypeAppString[0].c_str()).Install());

		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE | B_CREATE_FILE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetSupportedTypes(&types));
		CPPUNIT_ASSERT(SupportedTypesValue(types) == SupportedTypesValue(fTestTypes[0]));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == true);
		CheckSupportedTypesAttr(file, &fTestTypes[0]);
		SyncResources(appFileInfo);
		CheckSupportedTypesResource(file, &fTestTypes[0]);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[0].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[1].c_str(), fMimeTypeAppString[0].c_str()) == true);

		appFileInfo.SetTo(NULL);
		file.Unset();
		CPPUNIT_ASSERT_EQUAL(B_OK, BEntry(fTestFilePaths[0].c_str()).Remove());

		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetTo(fTestFilePaths[0].c_str(), B_READ_WRITE | B_CREATE_FILE));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));

		BMessage types2;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetSupportedTypes(&types2));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[0]) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&fMimeTestType[1]) == false);
		CheckNoAttr(file, kSupportedTypesAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kSupportedTypesAttribute);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[0].c_str(), fMimeTypeAppString[0].c_str()) == true);
		CPPUNIT_ASSERT(IsSupportingApp(fMimeTypeString[1].c_str(), fMimeTypeAppString[0].c_str()) == true);
	}

	void Valid_IsSupportedType_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[0].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[1].c_str()) == true);
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(fMimeTypeString[2].c_str()) == false);
	}

	void NullType_IsSupportedType_ReturnsFalse()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(NULL) == false);
	}

	void OctetStreamSupported_IsSupportedTypeImageGif_ReturnsTrue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BMimeType gifType("image/gif");
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(gifType.Type()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&gifType) == false);
		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_OK, types.AddString("types", "application/octet-stream"));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&types));
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(gifType.Type()) == true);
	}

	void Valid_Supports_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&fTestTypes[0]));

		BMimeType type1(fMimeTypeString[0].c_str());
		BMimeType type2(fMimeTypeString[1].c_str());
		BMimeType type3(fMimeTypeString[2].c_str());
		CPPUNIT_ASSERT(appFileInfo.Supports(&type1) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&type2) == true);
		CPPUNIT_ASSERT(appFileInfo.Supports(&type3) == false);

#ifndef TEST_R5
		CPPUNIT_ASSERT(appFileInfo.Supports(NULL) == false);
#endif
	}

	void NullType_Supports_ReturnsFalse()
	{
#ifndef TEST_R5
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT(appFileInfo.Supports(NULL) == false);
#endif
	}

	void OctetStreamSupported_SupportsImageGif_ReturnsFalse()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BMimeType gifType("image/gif");
		CPPUNIT_ASSERT(appFileInfo.IsSupportedType(gifType.Type()) == false);
		CPPUNIT_ASSERT(appFileInfo.Supports(&gifType) == false);
		BMessage types;
		CPPUNIT_ASSERT_EQUAL(B_OK, types.AddString("types", "application/octet-stream"));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSupportedTypes(&types));
		CPPUNIT_ASSERT(appFileInfo.Supports(&gifType) == false);
	}

	void NullIcon_GetIcon_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIcon(NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIcon(NULL, B_LARGE_ICON));
	}

	void Uninitialized_GetIcon_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetIcon(&icon2, B_LARGE_ICON));
	}

	void WrongIconDimensions_GetIcon_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIcon(&icon2, B_LARGE_ICON));
	}

	void NoIcon_GetIcon_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetIcon(&icon, B_MINI_ICON));
	}

	void SetGetResetGet_SetIcon_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconM[0], B_MINI_ICON));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconAttr(file, fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconM[1], B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon2, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon2));
		CheckIconAttr(file, fIconM[1]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM[1]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconL[0], B_LARGE_ICON));

		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon3, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon3));
		CheckIconAttr(file, fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconL[1], B_LARGE_ICON));
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon4));
		CheckIconAttr(file, fIconL[1]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL[1]);
	}

	// * bitmap color_space != B_CMAP8 => B_OK
	void BitmapColorSpaceNotCMAP8_GetIcon_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		BBitmap icon(BRect(0, 0, 15, 15), B_RGB32);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_RGB32);
		// SetBits() can be used, since there's no row padding for 16x16.
		icon2.SetBits(fIconM[1]->Bits(), fIconM[1]->BitsLength(), 0, B_CMAP8);
		CPPUNIT_ASSERT(IconEqual(&icon, &icon2));

// R5: Crashes for some weird reason in GetIcon().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon3, B_LARGE_ICON));
		BBitmap icon4(BRect(0, 0, 31, 31), B_RGB32);
		// SetBits() can be used, since there's no row padding for 32x32.
		icon4.SetBits(fIconL[1]->Bits(), fIconL[1]->BitsLength(), 0, B_CMAP8);
		CPPUNIT_ASSERT(IconEqual(&icon3, &icon4));
#endif
	}

	// status_t SetIcon(const BBitmap *icon, icon_size k)
	void NullIcon_SetIcon_UnsetsIconReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(NULL, B_MINI_ICON));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetIcon(&icon, B_MINI_ICON));
		CheckNoAttr(file, kMiniIconAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kMiniIconAttribute);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(NULL, B_LARGE_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetIcon(&icon2, B_LARGE_ICON));
		CheckNoAttr(file, kLargeIconAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kLargeIconAttribute);
	}

	void Uninitialized_SetIcon_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetIcon(&icon2, B_LARGE_ICON));
	}

	void WrongIconDimensions_SetIcon_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIcon(&icon2, B_LARGE_ICON));
	}

	void FileHasAppSignature_SetIcon_IconAlsoSetOnMimeType()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		// uninstalled signature
		BMimeType mimeType(fMimeTypeAppString[0].c_str());
		CPPUNIT_ASSERT(mimeType.IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconL[0], B_LARGE_ICON));
		CPPUNIT_ASSERT(mimeType.IsInstalled() == true);

		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, mimeType.GetIcon(&icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));

		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, mimeType.GetIcon(&icon2, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon2));
		// installed signature
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconM[1], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIcon(fIconL[1], B_LARGE_ICON));

		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, mimeType.GetIcon(&icon3, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon3));

		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, mimeType.GetIcon(&icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon4));
	}

	void Uninitialized_GetVersionInfo_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		version_info info;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		version_info info2;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetVersionInfo(&info2, B_SYSTEM_VERSION_KIND));
	}

	void NullInfo_GetVersionInfo_ReturnsBadValue()
	{
	#ifndef TEST_R5
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT(equals(appFileInfo.GetVersionInfo(NULL, B_APP_VERSION_KIND),
					   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(appFileInfo.GetVersionInfo(NULL, B_SYSTEM_VERSION_KIND),
					   B_BAD_ADDRESS, B_BAD_VALUE));
	#endif
	}

	void NoVersionInfo_GetVersionInfo_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		version_info info;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		version_info info2;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info2, B_SYSTEM_VERSION_KIND));
	}

	void Uninitialized_SetVersionInfo_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetVersionInfo(&fTestInfo[0], B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetVersionInfo(&fTestInfo[0], B_SYSTEM_VERSION_KIND));
	}

	void Valid_SetVersionInfo_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		version_info info;

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[0], B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[1], B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[0]);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[1]);

		version_info infos[] = { fTestInfo[0], fTestInfo[1] };
		CheckVersionInfoAttr(file, infos);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, infos);
	}

	void Null_SetVersionInfo_UnsetsBothKindsReturnsOk()
	{
// R5: Crashes when passing a NULL icon.
#ifndef TEST_R5
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[0], B_APP_VERSION_KIND));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(NULL, B_APP_VERSION_KIND));
		version_info info;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));
		CheckNoAttr(file, kVersionInfoAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kVersionInfoAttribute);
	
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[0], B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[0], B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(NULL, B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));
		CheckNoAttr(file, kVersionInfoAttribute);
		SyncResources(appFileInfo);
		CheckNoResource(file, kVersionInfoAttribute);
#endif
	}

	void SetGetResetGetAppKind_SetVersionInfo_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[0], B_APP_VERSION_KIND));

		version_info info;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[0]);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));
		version_info infos[] = { fTestInfo[0], fTestInfo[0] };
		CheckVersionInfoAttr(file, infos);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, infos);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[2], B_APP_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[2]);
		version_info infos2[] = { fTestInfo[2], fTestInfo[0] };
		CheckVersionInfoAttr(file, infos2);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, infos2);
	}

	void SetGetResetGetSystemKind_SetVersionInfo_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[1], B_SYSTEM_VERSION_KIND));

		version_info info;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[1]);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND));
		version_info infos[] = { fTestInfo[1], fTestInfo[1] };
		CheckVersionInfoAttr(file, infos);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, infos);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetVersionInfo(&fTestInfo[3], B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetVersionInfo(&info, B_SYSTEM_VERSION_KIND));
		CPPUNIT_ASSERT(info == fTestInfo[3]);
		version_info infos2[] = { fTestInfo[1], fTestInfo[3] };
		CheckVersionInfoAttr(file, infos2);
		SyncResources(appFileInfo);
		CheckVersionInfoResource(file, infos2);
	}

	void Uninitialized_GetIconForType_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
	}

	void NullIcon_GetIconForType_ReturnsBadValue()
	{
// R5: Crashes when passing a NULL icon.
#ifndef TEST_R5
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), NULL, B_LARGE_ICON));
#endif
	}

	void WrongIconDimensions_GetIconForType_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
	}

	void NoIcon_GetIconForType_ReturnsEntryNotFound()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
	}

	// * bitmap color_space != B_CMAP8 => B_OK
	void BitmapColorSpaceNotCMAP8_GetIconForType_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		BBitmap icon(BRect(0, 0, 15, 15), B_RGB32);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_RGB32);
		// SetBits() can be used, since there's no row padding for 16x16.
		icon2.SetBits(fIconM[1]->Bits(), fIconM[1]->BitsLength(), 0, B_CMAP8);
		CPPUNIT_ASSERT(IconEqual(&icon, &icon2));

// R5: Crashes for some weird reason in GetIconForType().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon3, B_LARGE_ICON));
		BBitmap icon4(BRect(0, 0, 31, 31), B_RGB32);
		// SetBits() can be used, since there's no row padding for 32x32.
		icon4.SetBits(fIconL[1]->Bits(), fIconL[1]->BitsLength(), 0, B_CMAP8);
		CPPUNIT_ASSERT(IconEqual(&icon3, &icon4));
#endif
	}

	void Valid_SetIconForType_ReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[0], B_MINI_ICON));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconM[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[0], B_LARGE_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconL[0]);
	}

	void Uninitialized_SetIconForType_ReturnsNoInit()
	{
		BAppFileInfo appFileInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
	}

	void WrongIconDimensions_SetIconForType_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
	}

	void SetGetResetGet_SetIconForTypeMiniIcon_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo(&file);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[0], B_MINI_ICON));

		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconM[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[1], B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon2));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconM[1]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconM[1]);
	}

	void SetGetResetGet_SetIconForTypeLargeIcon_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[0], B_LARGE_ICON));

		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconL[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[1], B_LARGE_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon2));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconL[1]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconL[1]);
	}

	void NullMiniIcon_SetIconForType_UnsetsIconReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[0], B_MINI_ICON));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), NULL, B_MINI_ICON));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		CheckNoIconForTypeAttr(file, fMimeTypeString[0].c_str(), B_MINI_ICON);
		SyncResources(appFileInfo);
		CheckNoIconForTypeResource(file, fMimeTypeString[0].c_str(), B_MINI_ICON);
	}

	void NullLargeIcon_SetIconForType_UnsetsIconReturnsOk()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[0], B_LARGE_ICON));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), NULL, B_LARGE_ICON));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_LARGE_ICON));
		CheckNoIconForTypeAttr(file, fMimeTypeString[0].c_str(), B_LARGE_ICON);
		SyncResources(appFileInfo);
		CheckNoIconForTypeResource(file, fMimeTypeString[0].c_str(), B_LARGE_ICON);
	}

	void NullType_SetIconForType_SetsStandardIcon()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(NULL, fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(NULL, fIconL[0], B_LARGE_ICON));
		// get mini icon via GetIcon()
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconAttr(file, fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM[0]);
		// get large via GetIcon()
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIcon(&icon2, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon2));
		CheckIconAttr(file, fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL[0]);
		// get mini icon via GetIconForType()
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(NULL, &icon3, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon3));
		CheckIconAttr(file, fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM[0]);
		// get large via GetIconForType()
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(NULL, &icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon4));
		CheckIconAttr(file, fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL[0]);
	}

	void TwoTypesSetGet_SetIconForType_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		// set icons for both types
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[0], B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[1].c_str(), fIconM[1], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[1].c_str(), fIconL[1], B_LARGE_ICON));
		// check first type
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconM[0]);
		// get large
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon2));
		CheckIconForTypeAttr(file, fMimeTypeString[0].c_str(), fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[0].c_str(), fIconL[0]);
		// check second type
		// get mini
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[1].c_str(), &icon3, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon3));
		CheckIconForTypeAttr(file, fMimeTypeString[1].c_str(), fIconM[1]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[1].c_str(), fIconM[1],
								 kMiniIconForTypeResourceID + 1);
		// get large
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[1].c_str(), &icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon4));
		CheckIconForTypeAttr(file, fMimeTypeString[1].c_str(), fIconL[1]);
		SyncResources(appFileInfo);
		CheckIconForTypeResource(file, fMimeTypeString[1].c_str(), fIconL[1],
								 kLargeIconForTypeResourceID + 1);
		// unset both icons
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), NULL, B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[1].c_str(), NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[1].c_str(), NULL, B_LARGE_ICON));
	}

	void InvalidType_SetIconForType_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIconForType(invalidTestType, fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIconForType(invalidTestType, fIconL[0], B_LARGE_ICON));
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIconForType(invalidTestType, &icon, B_MINI_ICON));
		CheckNoIconForTypeAttr(file, invalidTestType, B_LARGE_ICON);
		CheckNoIconForTypeResource(file, invalidTestType, B_LARGE_ICON);
		// get large
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.GetIconForType(invalidTestType, &icon3, B_LARGE_ICON));
		CheckNoIconForTypeAttr(file, invalidTestType, B_MINI_ICON);
		CheckNoIconForTypeResource(file, invalidTestType, B_MINI_ICON);
	}

	void TooLongType_SetIconForType_ReturnsBadValue()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIconForType(tooLongTestType, fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, appFileInfo.SetIconForType(tooLongTestType, fIconL[0], B_LARGE_ICON));
	}

	void FileHasAppSignature_SetIconForType_IconAlsoSetOnMimeType()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));
		// uninstalled signature
		BMimeType mimeType(fMimeTypeAppString[0].c_str());
		CPPUNIT_ASSERT(mimeType.IsInstalled() == false);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetSignature(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK,
			appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK,
			appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[0], B_LARGE_ICON));
		CPPUNIT_ASSERT(mimeType.IsInstalled() == true);
		CPPUNIT_ASSERT(BMimeType(fMimeTypeString[0].c_str()).IsInstalled() == false);
		// get mini
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK,
			mimeType.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		// get large
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK,
			mimeType.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon2));
		// installed signature
		CPPUNIT_ASSERT_EQUAL(B_OK,
			appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[1], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK,
			appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[1], B_LARGE_ICON));
		// get mini
		BBitmap icon3(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK,
			mimeType.GetIconForType(fMimeTypeString[0].c_str(), &icon3, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon3));
		// get large
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK,
			mimeType.GetIconForType(fMimeTypeString[0].c_str(), &icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon4));
	}

	void SetGetResetGet_SetIconForType_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		BAppFileInfo appFileInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetTo(&file));

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[0], B_MINI_ICON));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconAttr(file, fIconM[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconM[1], B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon2, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon2));
		CheckIconAttr(file, fIconM[1]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconM[1]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[0], B_LARGE_ICON));

		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon3, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon3));
		CheckIconAttr(file, fIconL[0]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL[0]);

		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.SetIconForType(fMimeTypeString[0].c_str(), fIconL[1], B_LARGE_ICON));
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, appFileInfo.GetIconForType(fMimeTypeString[0].c_str(), &icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon4));
		CheckIconAttr(file, fIconL[1]);
		SyncResources(appFileInfo);
		CheckIconResource(file, fIconL[1]);
	}

#if __GNUC__ > 2 // FIXME: after refactoring GCC2 fails with internal compiler error
	void ValidType_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		TypeValue v1(fMimeTypeString[0].c_str());
		TypeValue v2(fMimeTypeString[1].c_str());
		TypeValue v3(fMimeTypeString[2].c_str());
		InfoLocationTester<TypeValue, TypeSetter, TypeGetter, TypeChecker>(
			file, v1, v2, v3);
	}

	void ValidSignature_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		SignatureValue v1(fMimeTypeAppString[0].c_str());
		SignatureValue v2(fMimeTypeAppString[1].c_str());
		SignatureValue v3(fMimeTypeAppString[2].c_str());
		InfoLocationTester<SignatureValue, SignatureSetter, SignatureGetter,
						   SignatureChecker>(file, v1, v2, v3);
	}

	void ValidAppFlags_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		AppFlagsValue v1(B_SINGLE_LAUNCH);
		AppFlagsValue v2(B_MULTIPLE_LAUNCH);
		AppFlagsValue v3(B_BACKGROUND_APP);
		InfoLocationTester<AppFlagsValue, AppFlagsSetter, AppFlagsGetter,
						   AppFlagsChecker>(file, v1, v2, v3);
	}

	void ValidSupportedTypes_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		{
			BMimeType type(fMimeTypeAppString[0].c_str());
			if (!type.IsInstalled())
				CPPUNIT_ASSERT_EQUAL(B_OK, type.Install());
		}
		const char *supportedTypes1[] = { fMimeTypeString[0].c_str() };
		const char *supportedTypes2[] = { fMimeTypeString[1].c_str(), fMimeTypeString[2].c_str() };
		const char *supportedTypes3[] = { fMimeTypeString[2].c_str(), fMimeTypeString[1].c_str(), fMimeTypeString[0].c_str() };
		SupportedTypesValue v1(supportedTypes1, 1);
		SupportedTypesValue v2(supportedTypes2, 2);
		SupportedTypesValue v3(supportedTypes3, 3);
		InfoLocationTester<SupportedTypesValue, SupportedTypesSetter,
						   SupportedTypesGetter, SupportedTypesChecker>(
			file, v1, v2, v3);
	}

	void ValidIcon_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		IconValue v1(fIconM[0], fIconL[0]);
		IconValue v2(fIconM[1], fIconL[1]);
		IconValue v3(fIconM[2], fIconL[2]);
		InfoLocationTester<IconValue, IconSetter, IconGetter, IconChecker>(
			file, v1, v2, v3);
	}

	void ValidVersionInfo_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		VersionInfoValue v1(fTestInfo[0], fTestInfo[0]);
		VersionInfoValue v2(fTestInfo[1], fTestInfo[1]);
		VersionInfoValue v3(fTestInfo[2], fTestInfo[2]);
		InfoLocationTester<VersionInfoValue, VersionInfoSetter, VersionInfoGetter,
						   VersionInfoChecker>(file, v1, v2, v3);
	}

	void ValidIconForType_SetInfoLocation_ReturnsExpected()
	{
		BFile file(fTestFilePaths[0].c_str(), B_READ_WRITE);
		IconForTypeValue v1(fIconM[0], fIconL[0]);
		IconForTypeValue v2(fIconM[1], fIconL[1]);
		IconForTypeValue v3(fIconM[2], fIconL[2]);
		InfoLocationTester<IconForTypeValue, IconForTypeSetter, IconForTypeGetter,
						   IconForTypeChecker>(file, v1, v2, v3);
	}
#endif
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AppFileInfoTest, getTestSuiteName());
