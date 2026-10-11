/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <MimeTypes.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TypeConstants.h>
#include <fs_attr.h>

#include <StorageDefs.h>
#include <SupportDefs.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "InfoTest.h"

// test dirs/files/types
static const char* abstractTestEntry = "/tmp/testDir/abstract-entry";
static const char* invalidTestType = "invalid-mime-type";
static const char* tooLongTestType
	= "0123456789012345678901234567890123456789012345678901234567890123456789"
	  "0123456789012345678901234567890123456789012345678901234567890123456789"
	  "0123456789012345678901234567890123456789012345678901234567890123456789"
	  "0123456789012345678901234567890123456789012345678901234567890123456789";


class BApplication;
class BBitmap;

static void
CheckTrackerIcon(BNodeInfo& nodeInfo, entry_ref* ref, icon_size size,
	BBitmap* expected)
{
	BBitmap icon(size == B_MINI_ICON ? BRect(0, 0, 15, 15)
		: BRect(0, 0, 31, 31), B_CMAP8);
	CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetTrackerIcon(&icon, size));
	CPPUNIT_ASSERT(InfoTest::IconEqual(expected, &icon));

	BBitmap staticIcon(icon.Bounds(), B_CMAP8);
	CPPUNIT_ASSERT_EQUAL(B_OK, BNodeInfo::GetTrackerIcon(ref, &staticIcon, size));
	CPPUNIT_ASSERT(InfoTest::IconEqual(expected, &staticIcon));
}


class NodeInfoTest : public InfoTest {
	CPPUNIT_TEST_SUITE(NodeInfoTest);
	CPPUNIT_TEST(DefaultConstructor_ReturnsNoInit);
	CPPUNIT_TEST(NullNode_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(InvalidNode_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(ValidNode_Constructor_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetToNullNode_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_SetToInvalidNode_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_SetToValidNode_ReturnsOk);
	CPPUNIT_TEST(Initialized_SetToNullNode_ReturnsBadValue);
	CPPUNIT_TEST(Initialized_SetToValidNode_ReturnsOk);
	CPPUNIT_TEST(Null_GetType_ReturnsError);
	CPPUNIT_TEST(Uninitialized_GetType_ReturnsNoInit);
	CPPUNIT_TEST(NoType_GetType_ReturnsNotFound);
	CPPUNIT_TEST(Type_SetGetResetGet_ReturnsOk);
	CPPUNIT_TEST(Null_SetType_UnsetsType);
	CPPUNIT_TEST(Uninitialized_SetType_ReturnsNoInit);
	CPPUNIT_TEST(InvalidType_SetType_ReturnsOk);
	CPPUNIT_TEST(TooLongType_SetType_ReturnsError);
	CPPUNIT_TEST(Null_GetIcon_ReturnsError);
	CPPUNIT_TEST(Uninitialized_GetIcon_ReturnsNoInit);
	CPPUNIT_TEST(DimensionMismatch_GetIcon_ReturnsError);
	CPPUNIT_TEST(NoIcon_GetIcon_ReturnsNotFound);
	CPPUNIT_TEST(Icon_SetGetResetGet_ReturnsOk);
	//CPPUNIT_TEST(NonCMAP8_GetIcon_ReturnsOk);
	CPPUNIT_TEST(Null_SetIcon_UnsetsIcon);
	CPPUNIT_TEST(Uninitialized_SetIcon_ReturnsNoInit);
	CPPUNIT_TEST(DimensionMismatch_SetIcon_ReturnsError);
	CPPUNIT_TEST(Null_GetPreferredApp_ReturnsError);
	CPPUNIT_TEST(Uninitialized_GetPreferredApp_ReturnsNoInit);
	CPPUNIT_TEST(NoPreferredApp_GetPreferredApp_ReturnsNotFound);
	CPPUNIT_TEST(PreferredApp_SetGetResetGet_ReturnsOk);
	CPPUNIT_TEST(Null_SetPreferredApp_UnsetsPreferredApp);
	CPPUNIT_TEST(Uninitialized_SetPreferredApp_ReturnsNoInit);
	CPPUNIT_TEST(InvalidType_SetPreferredApp_ReturnsOk);
	CPPUNIT_TEST(TooLongType_SetPreferredApp_ReturnsError);
	CPPUNIT_TEST(Null_GetAppHint_ReturnsError);
	CPPUNIT_TEST(Uninitialized_GetAppHint_ReturnsNoInit);
	CPPUNIT_TEST(NoAppHint_GetAppHint_ReturnsNotFound);
	CPPUNIT_TEST(AppHint_SetGetResetGet_ReturnsOk);
	CPPUNIT_TEST(Null_SetAppHint_UnsetsAppHint);
	CPPUNIT_TEST(Uninitialized_SetAppHint_ReturnsNoInit);
	CPPUNIT_TEST(NonExistentEntry_SetAppHint_ReturnsOk);
	// CPPUNIT_TEST(NullRef_GetTrackerIcon_ReturnsBadValue);
	// CPPUNIT_TEST(Uninitialized_GetTrackerIcon_ReturnsNoInit);
	// CPPUNIT_TEST(NullIcon_GetTrackerIcon_ReturnsBadValue);
	// CPPUNIT_TEST(DimensionMismatch_GetTrackerIcon_ReturnsBadValue);
	// CPPUNIT_TEST(UntypedFile_GetTrackerIcon_ReturnsOctetStreamIcon);
	// CPPUNIT_TEST(InvalidType_GetTrackerIcon_ReturnsFileIcon);
	// CPPUNIT_TEST(FileIcon_GetTrackerIcon_ReturnsFileIcon);
	// CPPUNIT_TEST(PreferredAppIcon_GetTrackerIcon_ReturnsAppIcon);
	// CPPUNIT_TEST(TypeIcon_GetTrackerIcon_ReturnsTypeIcon);
	// CPPUNIT_TEST(PreferredAppTypeIcon_GetTrackerIcon_ReturnsAppIcon);
	// CPPUNIT_TEST(NoConfiguredIcons_GetTrackerIcon_ReturnsOctetStreamIcon);
	// CPPUNIT_TEST(OtherEntryTypes_GetTrackerIcon_ReturnsOk);
	CPPUNIT_TEST_SUITE_END();

	BApplication* fApplication;

public:
	NodeInfoTest() : InfoTest("application/x-vnd.obos.node-file-info-test")
	{
	}

	void setUp()
	{
		fApplication = new BApplication(kMimeTypeBase);

		InfoTest::setUp();
	}

	void tearDown()
	{
		InfoTest::tearDown();

		delete fApplication;
		fApplication = NULL;

	}

	void DefaultConstructor_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.InitCheck());
	}

	void NullNode_Constructor_ReturnsBadValue()
	{
		BNodeInfo nodeInfo(NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.InitCheck());
	}

	void InvalidNode_Constructor_ReturnsBadValue()
	{
		BNode node;
		BNodeInfo nodeInfo(&node);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.InitCheck());
	}

	void ValidNode_Constructor_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.InitCheck());
	}

	void Uninitialized_SetToNullNode_ReturnsBadValue()
	{
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetTo(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.InitCheck());
	}

	void Uninitialized_SetToInvalidNode_ReturnsBadValue()
	{
		BNode node;
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.InitCheck());
	}

	void Uninitialized_SetToValidNode_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.InitCheck());
	}

	void Initialized_SetToNullNode_ReturnsBadValue()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.InitCheck());

		// reinit with NULL node
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetTo(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.InitCheck());
	}

	void Initialized_SetToValidNode_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.InitCheck());

		// reinit with valid node
		BNode node2(fTestFilePaths[1].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node2));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.InitCheck());
	}

	void Null_GetType_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT(equals(nodeInfo.GetType(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Uninitialized_GetType_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		char type[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetType(type));
	}

	void NoType_GetType_ReturnsNotFound()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		char type[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetType(type));
	}

	void Type_SetGetResetGet_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[0].c_str()));
		// get
		char type[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetType(type));
		CPPUNIT_ASSERT(strcmp(fMimeTypeString[0].c_str(), type) == 0);
		CheckTypeAttr(node, fMimeTypeString[0].c_str());
		// reset
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[1].c_str()));
		// get
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetType(type));
		CPPUNIT_ASSERT(strcmp(fMimeTypeString[1].c_str(), type) == 0);
		CheckTypeAttr(node, fMimeTypeString[1].c_str());
	}

	void Null_SetType_UnsetsType()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(NULL));
		// get
		char type[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetType(type));
		CheckNoAttr(node, kTypeAttribute);
	}

	void Uninitialized_SetType_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.SetType(fMimeTypeString[0].c_str()));
	}

	void InvalidType_SetType_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(invalidTestType));
		// get
		char type[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetType(type));
		CPPUNIT_ASSERT(strcmp(invalidTestType, type) == 0);
		CheckTypeAttr(node, invalidTestType);
	}

	void TooLongType_SetType_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// remove attr first
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(NULL));
// R5: Doesn't complain when setting a too long string.
// Haiku: Handles this as an error case.
#ifdef TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(tooLongTestType));
		// get
		char type[1024];
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetType(type));
		// R5: Returns a string one character shorter than the original string
		// CPPUNIT_ASSERT(strcmp(tooLongTestType, type) == 0);
		CheckTypeAttr(node, tooLongTestType);
#else
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetType(tooLongTestType));
		CheckNoAttr(node, kTypeAttribute);
#endif
	}

	void Null_GetIcon_ReturnsError()
	{
		// * NULL icon => B_BAD_VALUE
// R5: Crashes when passing a NULL icon.
#ifndef TEST_R5
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetIcon(NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetIcon(NULL, B_LARGE_ICON));
#endif
	}

	void Uninitialized_GetIcon_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetIcon(&icon2, B_LARGE_ICON));
	}

	void DimensionMismatch_GetIcon_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetIcon(&icon2, B_LARGE_ICON));
	}

	void NoIcon_GetIcon_ReturnsNotFound()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetIcon(&icon, B_MINI_ICON));
	}

	void Icon_SetGetResetGet_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// mini
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconM[0], B_MINI_ICON));
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetIcon(&icon, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[0], &icon));
		CheckIconAttr(node, fIconM[0]);
		// reset
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconM[1], B_MINI_ICON));
		// get
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetIcon(&icon2, B_MINI_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconM[1], &icon2));
		CheckIconAttr(node, fIconM[1]);
		// large
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconL[0], B_LARGE_ICON));
		// get
		BBitmap icon3(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetIcon(&icon3, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[0], &icon3));
		CheckIconAttr(node, fIconL[0]);
		// reset
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconL[1], B_LARGE_ICON));
		// get
		BBitmap icon4(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetIcon(&icon4, B_LARGE_ICON));
		CPPUNIT_ASSERT(IconEqual(fIconL[1], &icon4));
		CheckIconAttr(node, fIconL[1]);
	}

	void NonCMAP8_GetIcon_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconM[1], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconL[1], B_LARGE_ICON));

		// mini
		BBitmap icon(BRect(0, 0, 15, 15), B_RGB32);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_RGB32);
		// SetBits() can be used, since there's no row padding for 16x16.
		icon2.SetBits(fIconM[1]->Bits(), fIconM[1]->BitsLength(), 0, B_CMAP8);
		CPPUNIT_ASSERT(IconEqual(&icon, &icon2));
		// large
// R5: Crashes for some weird reason in GetIcon().
#ifndef TEST_R5
		BBitmap icon3(BRect(0, 0, 31, 31), B_RGB32);
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetIcon(&icon3, B_LARGE_ICON));
		BBitmap icon4(BRect(0, 0, 31, 31), B_RGB32);
		// SetBits() can be used, since there's no row padding for 32x32.
		icon4.SetBits(fIconL[1]->Bits(), fIconL[1]->BitsLength(), 0, B_CMAP8);
		CPPUNIT_ASSERT(IconEqual(&icon3, &icon4));
#endif
	}

	void Null_SetIcon_UnsetsIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// mini
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(NULL, B_MINI_ICON));
		// get
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetIcon(&icon, B_MINI_ICON));
		CheckNoAttr(node, kMiniIconAttribute);
		// large
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(NULL, B_LARGE_ICON));
		// get
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetIcon(&icon2, B_LARGE_ICON));
		CheckNoAttr(node, kLargeIconAttribute);
	}

	void Uninitialized_SetIcon_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		BBitmap icon(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.SetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.SetIcon(&icon2, B_LARGE_ICON));
	}

	void DimensionMismatch_SetIcon_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		BBitmap icon(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetIcon(&icon, B_MINI_ICON));
		BBitmap icon2(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetIcon(&icon2, B_LARGE_ICON));
	}

	void Null_GetPreferredApp_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT(equals(nodeInfo.GetPreferredApp(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Uninitialized_GetPreferredApp_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		char signature[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetPreferredApp(signature));
	}

	void NoPreferredApp_GetPreferredApp_ReturnsNotFound()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		char signature[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetPreferredApp(signature));
	}

	void PreferredApp_SetGetResetGet_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(fMimeTypeAppString[0].c_str()));
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetPreferredApp(signature));
		CPPUNIT_ASSERT(strcmp(fMimeTypeAppString[0].c_str(), signature) == 0);
		CheckPreferredAppAttr(node, fMimeTypeAppString[0].c_str());
		// reset
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(fMimeTypeAppString[1].c_str()));
		// get
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetPreferredApp(signature));
		CPPUNIT_ASSERT(strcmp(fMimeTypeAppString[1].c_str(), signature) == 0);
		CheckPreferredAppAttr(node, fMimeTypeAppString[1].c_str());
	}

	void Null_SetPreferredApp_UnsetsPreferredApp()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(NULL));
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetPreferredApp(signature));
		CheckNoAttr(node, kPreferredAppAttribute);
	}

	void Uninitialized_SetPreferredApp_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.SetPreferredApp(fMimeTypeAppString[0].c_str()));
	}

	void InvalidType_SetPreferredApp_ReturnsOk()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(invalidTestType));
		// get
		char signature[B_MIME_TYPE_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetPreferredApp(signature));
		CPPUNIT_ASSERT(strcmp(invalidTestType, signature) == 0);
		CheckPreferredAppAttr(node, invalidTestType);
	}

	void TooLongType_SetPreferredApp_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// unset
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(NULL));
		// try to set
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.SetPreferredApp(tooLongTestType));
		// get
		char signature[1024];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetPreferredApp(signature));
		CheckNoAttr(node, kPreferredAppAttribute);
	}

	void Null_GetAppHint_ReturnsError()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetAppHint(NULL));
	}

	void Uninitialized_GetAppHint_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetAppHint(&ref));
	}

	void NoAppHint_GetAppHint_ReturnsNotFound()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetAppHint(&ref));
	}

	void AppHint_SetGetResetGet_ReturnsOk()
	{
		// init test refs
		entry_ref testRef1, testRef2;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(fTestFilePaths[2].c_str(), &testRef1));
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(fTestFilePaths[3].c_str(), &testRef2));

		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// set
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetAppHint(&testRef1));
		// get
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetAppHint(&ref));
		CPPUNIT_ASSERT(ref == testRef1);
		CheckAppHintAttr(node, &testRef1);
		// reset
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetAppHint(&testRef2));
		// get
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetAppHint(&ref));
		CPPUNIT_ASSERT(ref == testRef2);
		CheckAppHintAttr(node, &testRef2);
	}

	void Null_SetAppHint_UnsetsAppHint()
	{
		entry_ref testRef1;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(fTestFilePaths[2].c_str(), &testRef1));

		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetAppHint(&testRef1));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetAppHint(NULL));
		// get
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, nodeInfo.GetAppHint(&ref));
		CheckNoAttr(node, kAppHintAttribute);
	}

	void Uninitialized_SetAppHint_ReturnsNoInit()
	{
		entry_ref testRef1;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(fTestFilePaths[2].c_str(), &testRef1));

		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.SetAppHint(&testRef1));
	}

	void NonExistentEntry_SetAppHint_ReturnsOk()
	{
		entry_ref abstractRef;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(abstractTestEntry, &abstractRef));

		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetTo(&node));
		// invalid ref
		entry_ref invalidRef;
		CPPUNIT_ASSERT(nodeInfo.SetAppHint(&invalidRef) != B_OK);
		// abstract ref
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetAppHint(&abstractRef));
		// get
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.GetAppHint(&ref));
		CPPUNIT_ASSERT(ref == abstractRef);
		CheckAppHintAttr(node, &abstractRef);
	}

	void NullRef_GetTrackerIcon_ReturnsBadValue()
	{
		BBitmap mini(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BNodeInfo::GetTrackerIcon(NULL, &mini, B_MINI_ICON));
		BBitmap large(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BNodeInfo::GetTrackerIcon(NULL, &large, B_LARGE_ICON));
	}

	void Uninitialized_GetTrackerIcon_ReturnsNoInit()
	{
		BNodeInfo nodeInfo;
		BBitmap mini(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetTrackerIcon(&mini, B_MINI_ICON));
		BBitmap large(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, nodeInfo.GetTrackerIcon(&large, B_LARGE_ICON));
	}

	void NullIcon_GetTrackerIcon_ReturnsBadValue()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetTrackerIcon(NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetTrackerIcon(NULL, B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BNodeInfo::GetTrackerIcon(&ref, NULL, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BNodeInfo::GetTrackerIcon(&ref, NULL, B_LARGE_ICON));
	}

	void DimensionMismatch_GetTrackerIcon_ReturnsBadValue()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BBitmap large(BRect(0, 0, 31, 31), B_CMAP8);
		BBitmap mini(BRect(0, 0, 15, 15), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetTrackerIcon(&large, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, nodeInfo.GetTrackerIcon(&mini, B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BNodeInfo::GetTrackerIcon(&ref, &large, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, BNodeInfo::GetTrackerIcon(&ref, &mini, B_LARGE_ICON));
	}

	void UntypedFile_GetTrackerIcon_ReturnsOctetStreamIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconL[0], B_LARGE_ICON));
		BMimeType mime(B_FILE_MIME_TYPE);
		BBitmap mini(BRect(0, 0, 15, 15), B_CMAP8);
		BBitmap large(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, mime.GetIcon(&mini, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, mime.GetIcon(&large, B_LARGE_ICON));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, &mini);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, &large);
	}

	void InvalidType_GetTrackerIcon_ReturnsFileIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconL[0], B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(invalidTestType));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, fIconM[0]);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, fIconL[0]);
	}

	void FileIcon_GetTrackerIcon_ReturnsFileIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconM[0], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetIcon(fIconL[0], B_LARGE_ICON));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, fIconM[0]);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, fIconL[0]);
	}

	void PreferredAppIcon_GetTrackerIcon_ReturnsAppIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BMimeType app(fMimeTypeAppString[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, app.Install());
		CPPUNIT_ASSERT_EQUAL(B_OK, app.SetIconForType(fMimeTypeString[0].c_str(), fIconM[1], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, app.SetIconForType(fMimeTypeString[0].c_str(), fIconL[1], B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetPreferredApp(fMimeTypeAppString[0].c_str()));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, fIconM[1]);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, fIconL[1]);
	}

	void TypeIcon_GetTrackerIcon_ReturnsTypeIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BMimeType type(fMimeTypeString[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, type.Install());
		CPPUNIT_ASSERT_EQUAL(B_OK, type.SetIcon(fIconM[2], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, type.SetIcon(fIconL[2], B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[0].c_str()));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, fIconM[2]);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, fIconL[2]);
	}

	void PreferredAppTypeIcon_GetTrackerIcon_ReturnsAppIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BMimeType type(fMimeTypeString[0].c_str());
		BMimeType app(fMimeTypeAppString[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, type.Install());
		CPPUNIT_ASSERT_EQUAL(B_OK, app.Install());
		CPPUNIT_ASSERT_EQUAL(B_OK, type.SetPreferredApp(fMimeTypeAppString[0].c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, app.SetIconForType(fMimeTypeString[0].c_str(), fIconM[3], B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, app.SetIconForType(fMimeTypeString[0].c_str(), fIconL[3], B_LARGE_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.SetType(fMimeTypeString[0].c_str()));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, fIconM[3]);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, fIconL[3]);
	}

	void NoConfiguredIcons_GetTrackerIcon_ReturnsOctetStreamIcon()
	{
		BNode node(fTestFilePaths[0].c_str());
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry(fTestFilePaths[0].c_str());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BMimeType mime(B_FILE_MIME_TYPE);
		BBitmap mini(BRect(0, 0, 15, 15), B_CMAP8);
		BBitmap large(BRect(0, 0, 31, 31), B_CMAP8);
		CPPUNIT_ASSERT_EQUAL(B_OK, mime.GetIcon(&mini, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, mime.GetIcon(&large, B_LARGE_ICON));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, &mini);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, &large);
	}

	void OtherEntryTypes_GetTrackerIcon_ReturnsOk()
	{
		BBitmap mini(BRect(0, 0, 15, 15), B_CMAP8);
		BBitmap large(BRect(0, 0, 31, 31), B_CMAP8);
		BMimeType directory(B_DIRECTORY_MIME_TYPE);
		BMimeType volume(B_VOLUME_MIME_TYPE);
		CPPUNIT_ASSERT_EQUAL(B_OK, directory.GetIcon(&mini, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, directory.GetIcon(&large, B_LARGE_ICON));
		BNode node("/");
		BNodeInfo nodeInfo(&node);
		entry_ref ref;
		BEntry entry("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, nodeInfo.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.GetIcon(&mini, B_MINI_ICON));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.GetIcon(&large, B_LARGE_ICON));
		CheckTrackerIcon(nodeInfo, &ref, B_MINI_ICON, &mini);
		CheckTrackerIcon(nodeInfo, &ref, B_LARGE_ICON, &large);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(NodeInfoTest, getTestSuiteName());
