/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _STORAGE_INFO_TEST_H_
#define _STORAGE_INFO_TEST_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include <Bitmap.h>
#include <MimeType.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <SupportDefs.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "shared/AutoDeleter.h"

#include "BasicTest.h"


class InfoTest : public BasicTest {
public:
	InfoTest(const char* mimeTypeBase)
		:
		kMimeTypeBase(mimeTypeBase),
		kTypeAttribute("BEOS:TYPE"),
		kMiniIconAttribute("BEOS:M:STD_ICON"),
		kLargeIconAttribute("BEOS:L:STD_ICON"),
		kPreferredAppAttribute("BEOS:PREF_APP"),
		kAppHintAttribute("BEOS:PPATH")
	{
		for (int i = 0; i < 4; i++) {
			std::ostringstream mimeType, appMimeType;
			mimeType << kMimeTypeBase << i;
			appMimeType << kMimeTypeBase << "-app" << i;
			fMimeTypeString[i] = mimeType.str();
			fMimeTypeAppString[i] = appMimeType.str();
		}
	}

	virtual ~InfoTest() {
	}

	virtual void setUp() {
		BasicTest::setUp();

		for (int i = 0; i < 4; i++) {
			fTestFilePaths[i] = CreateTemporaryFile();
			fIconM[i] = CreateIcon(B_MINI_ICON, i);
			fIconL[i] = CreateIcon(B_LARGE_ICON, i);
		}
	}

	virtual void tearDown() {
		for (int i = 0; i < 4; i++) {
			delete fIconM[i];
			fIconM[i] = NULL;
			delete fIconL[i];
			fIconL[i] = NULL;

			BMimeType mime1(fMimeTypeString[i].c_str());
			BMimeType mime2(fMimeTypeAppString[i].c_str());
			mime1.Delete();
			mime2.Delete();

			unlink(fTestFilePaths[i].c_str());
			fTestFilePaths[i].clear();
		}

		BasicTest::tearDown();
	}

	static bool
	IconEqual(const BBitmap* icon1, const BBitmap* icon2)
	{
		return icon1->Bounds() == icon2->Bounds() && icon1->BitsLength() == icon2->BitsLength()
			&& memcmp(icon1->Bits(), icon2->Bits(), icon1->BitsLength()) == 0;
	}

	static void
	CheckAttr(BNode& node, const char* name, type_code type, const void* data, int32 dataSize)
	{
		attr_info info;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.GetAttrInfo(name, &info));
		CPPUNIT_ASSERT(info.type == type);
		CPPUNIT_ASSERT(info.size == dataSize);
		char* buffer = new char[dataSize];
		BPrivate::ArrayDeleter<char> deleter(buffer);
		CPPUNIT_ASSERT(node.ReadAttr(name, type, 0, buffer, dataSize) == dataSize);
		CPPUNIT_ASSERT(memcmp(buffer, data, dataSize) == 0);
	}

	static void
	CheckNoAttr(BNode& node, const char* name)
	{
		attr_info info;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.GetAttrInfo(name, &info));
	}

protected:
	const char* kMimeTypeBase;
	const char* kTypeAttribute;
	const char* kMiniIconAttribute;
	const char* kLargeIconAttribute;
	const char* kPreferredAppAttribute;
	const char* kAppHintAttribute;

	std::string		fTestFilePaths[4];
	std::string		fMimeTypeString[4];
	std::string		fMimeTypeAppString[4];
	BBitmap			*fIconL[4];
	BBitmap			*fIconM[4];

	void
	CheckTypeAttr(BNode& node, const char* data)
	{
		CheckAttr(node, kTypeAttribute, B_MIME_STRING_TYPE, data, strlen(data) + 1);
	}

	void
	CheckIconAttr(BNode& node, BBitmap* data)
	{
		const char* attribute = NULL;
		uint32 type = 0;
		switch (data->Bounds().IntegerWidth()) {
			case 15:
				attribute = kMiniIconAttribute;
				type = B_MINI_ICON_TYPE;
				break;
			case 31:
				attribute = kLargeIconAttribute;
				type = B_LARGE_ICON_TYPE;
				break;
			default:
				CPPUNIT_ASSERT(false);
				break;
		}
		CheckAttr(node, attribute, type, data->Bits(), data->BitsLength());
	}

	void
	CheckPreferredAppAttr(BNode& node, const char* data)
	{
		CheckAttr(node, kPreferredAppAttribute, B_MIME_STRING_TYPE, data, strlen(data) + 1);
	}

	void
	CheckAppHintAttr(BNode& node, const entry_ref* ref)
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(ref));
		const char* data = path.Path();
		// R5: Attribute is of type B_MIME_STRING_TYPE though it contains a path name!
		CheckAttr(node, kAppHintAttribute, B_MIME_STRING_TYPE, data, strlen(data) + 1);
	}

private:
	BBitmap*
	CreateIcon(icon_size size, int fill)
	{
		BBitmap* icon = NULL;
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
};


#endif
