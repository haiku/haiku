/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <StorageDefs.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <string>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"


class PathTest : public BasicTest {
	CPPUNIT_TEST_SUITE(PathTest);
	CPPUNIT_TEST(DefaultConstructor_ReturnsNoInit);
	CPPUNIT_TEST(Root_Constructor_ReturnsSame);
	CPPUNIT_TEST(Boot_Constructor_ReturnsSame);
	CPPUNIT_TEST(AbsoluteNonExistingNormalized_Constructor_ReturnsSame);
	CPPUNIT_TEST(Root_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(Boot_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(AbsoluteNonExisting_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(RelativeExistingNormalized_Constructor_ReturnsSame);
	CPPUNIT_TEST(RelativeNonExistingNormalized_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(RelativeExisting_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(RelativeNonExisting_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(RootLeaf_Constructor_ReturnsSame);
	CPPUNIT_TEST(BootLeaf_Constructor_ReturnsSame);
	CPPUNIT_TEST(NonExistingLeaf_Constructor_ReturnsSame);
	CPPUNIT_TEST(RootLeaf_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(BootLeaf_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(NonExistingLeaf_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(RelativeExistingLeaf_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(RelativeNonExistingLeaf_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(RelativeLeafWithTrailingSlash_Constructor_ReturnsNormalized);
	CPPUNIT_TEST(AbsoluteLeaf_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(NullPathAndLeaf_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(NullPath_Constructor_ReturnsBadValue);
	CPPUNIT_TEST(Root_Constructor_ReturnsOk);
	CPPUNIT_TEST(Boot_Constructor_ReturnsOk);
	CPPUNIT_TEST(Entry_Constructor_ReturnsOk);
	CPPUNIT_TEST(Ref_Constructor_ReturnsOk);
	CPPUNIT_TEST(RootForceNormalized_DirPathConstructor_ReturnsSame);
	CPPUNIT_TEST(BootForceNormalized_DirPathConstructor_ReturnsSame);
	CPPUNIT_TEST(RootLeaf_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(BootLeaf_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(LeafAutoNormalization_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(AbsoluteLeaf_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(UninitializedDir_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(BadlyInitializedDir_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndLeafName_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndNullLeaf_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(Boot_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(Abstract_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(UninitializedEntry_EntryConstructor_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitializedEntry_EntryConstructor_ReturnsNoInit);
	CPPUNIT_TEST(NullEntry_EntryConstructor_ReturnsBadValue);
	CPPUNIT_TEST(Boot_RefConstructor_ReturnsOk);
	CPPUNIT_TEST(Abstract_RefConstructor_ReturnsOk);
	CPPUNIT_TEST(NullRef_RefConstructor_ReturnsBadValue);
	CPPUNIT_TEST(RootForceNormalized_Constructor_ReturnsSame);
	CPPUNIT_TEST(BootForceNormalized_Constructor_ReturnsSame);
	CPPUNIT_TEST(NonExistingForceNormalized_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(RelativeForceNormalized_Constructor_ReturnsOk);
	CPPUNIT_TEST(RelativeNonExistingForceNormalized_Constructor_ReturnsNotFound);
	CPPUNIT_TEST(NonExistingLeafForceNormalized_Constructor_ReturnsNotFound);

	CPPUNIT_TEST(Root_SetTo_ReturnsSame);
	CPPUNIT_TEST(Boot_SetTo_ReturnsSame);
	CPPUNIT_TEST(AbsoluteLeafNormalized_SetTo_ReturnsSame);
	CPPUNIT_TEST(AbsoluteExisting_SetTo_ReturnsNormalized);
	CPPUNIT_TEST(AbsoluteNonExisting_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(AbsoluteNonExistingNormalized_SetTo_ReturnsSame);
	CPPUNIT_TEST(RelativeExisting_SetTo_ReturnsNormalized);
	CPPUNIT_TEST(RelativeNonExistingNormalized_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(RelativeNonExistingLeafNormalized_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(SelfPath_SetTo_ReturnsSame);
	CPPUNIT_TEST(ParentPath_SetTo_ReturnsSame);
	CPPUNIT_TEST(LeafPath_SetTo_ReturnsSame);
	CPPUNIT_TEST(BadArgs_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(BootLeaf_SetTo_ReturnsOk);
	CPPUNIT_TEST(NonNormalized_NonExistingLeaf_SetTo_ReturnsOk);
	CPPUNIT_TEST(RootDotLeaf_SetTo_ReturnsNormalized);
	CPPUNIT_TEST(NonExistingLeafAutoNormalization_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(NonExistingLeafForceNormalized_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(RelativeNonExistingLeaf_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(RelativeAutoNormalizationLeaf_SetTo_ReturnsNormalized);
	CPPUNIT_TEST(ForceNormalized_SetTo_ReturnsOk);
	CPPUNIT_TEST(NonExistingForceNormalized_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(RelativeForceNormalized_SetTo_ReturnsOk);
	CPPUNIT_TEST(RelativeNonExistingForceNormalized_SetTo_ReturnsNotFound);
	CPPUNIT_TEST(NullArgs_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(RootForceNormalized_SetToDirPath_ReturnsSame);
	CPPUNIT_TEST(BootForceNormalized_SetToDirPath_ReturnsSame);
	CPPUNIT_TEST(RootLeaf_SetToDirPath_ReturnsOk);
	CPPUNIT_TEST(BootLeaf_SetToDirPath_ReturnsOk);
	CPPUNIT_TEST(BootAutoNormalizationLeaf_SetToDirPath_ReturnsOk);
	CPPUNIT_TEST(DirAbsoluteLeaf_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(DirUninitialized_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(DirBadlyInitialized_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(DirNull_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(Root_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(Boot_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(AbstractEntry_SetTo_ReturnsOk);
	CPPUNIT_TEST(EntryUninitialized_SetTo_ReturnsNoInit);
	CPPUNIT_TEST(EntryBadlyInitialized_SetTo_ReturnsNoInit);
	CPPUNIT_TEST(EntryNull_SetTo_ReturnsBadValue);
	CPPUNIT_TEST(Root_SetToRef_ReturnsOk);
	CPPUNIT_TEST(Boot_SetToRef_ReturnsOk);
	CPPUNIT_TEST(AbstractEntry_SetToRef_ReturnsOk);
	CPPUNIT_TEST(RefNull_SetTo_ReturnsBadValue);

	CPPUNIT_TEST(Uninitialized_Append_ReturnsBadValue);
	CPPUNIT_TEST(Hierarchy_Append_ReturnsExpected);
	CPPUNIT_TEST(ForceNormalization_Append_ReturnsExpected);
	CPPUNIT_TEST(NullLeaf_Append_ReturnsSame);
	CPPUNIT_TEST(AbsoluteLeaf_Append_ReturnsBadValue);
	CPPUNIT_TEST(EmptyLeaf_Append_ReturnsSame);

	CPPUNIT_TEST(Uninitialized_Leaf_ReturnsNull);
	CPPUNIT_TEST(Root_Leaf_ReturnsEmpty);
	CPPUNIT_TEST(ExistingBoot_Leaf_ReturnsName);
	CPPUNIT_TEST(ExistingHome_Leaf_ReturnsName);
	CPPUNIT_TEST(ExistingDesktop_Leaf_ReturnsName);
	CPPUNIT_TEST(NonExistingPath_Leaf_ReturnsName);
	CPPUNIT_TEST(NonExistingDirectory_Leaf_ReturnsName);

	CPPUNIT_TEST(Uninitialized_GetParent_ReturnsNoInit);
	CPPUNIT_TEST(Root_GetParent_ReturnsNotFound);
	CPPUNIT_TEST(Boot_GetParent_ReturnsRoot);
	CPPUNIT_TEST(Home_GetParent_ReturnsBoot);
	CPPUNIT_TEST(Desktop_GetParent_ReturnsHome);
	CPPUNIT_TEST(NonExisting_GetParent_ReturnsRoot);
	CPPUNIT_TEST(NonExistingDirectory_GetParent_ReturnsParent);
	CPPUNIT_TEST(Sequence_GetParent_ReturnsExpected);
	CPPUNIT_TEST(NullArg_GetParent_ReturnsBadValue);

	CPPUNIT_TEST(Uninitialized_BPathComparison_ReturnsEqual);
	CPPUNIT_TEST(Initialized_BPathComparison_ReturnsDifferent);
	CPPUNIT_TEST(PathNames_BPathComparison_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_CharPtrComparison_ReturnsExpected);
	CPPUNIT_TEST(PathNames_CharPtrComparison_ReturnsExpected);
	CPPUNIT_TEST(Null_CharPtrComparison_ReturnsExpected);

	CPPUNIT_TEST(Uninitialized_CopyConstructor_ReturnsNoInit);
	CPPUNIT_TEST(Initialized_CopyConstructor_ReturnsCopy);
	CPPUNIT_TEST(Uninitialized_BPathAssignment_ReturnsNoInit);
	CPPUNIT_TEST(InitializedToUninitialized_BPathAssignment_ReturnsNoInit);
	CPPUNIT_TEST(Initialized_BPathAssignment_ReturnsCopy);
	CPPUNIT_TEST(CharPtr_Assignment_ReturnsPath);
	CPPUNIT_TEST(NullCharPtr_Assignment_ReturnsNoInit);

	CPPUNIT_TEST(Trivial_Flattenable_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_FlattenUnflatten_ReturnsNoInit);
	CPPUNIT_TEST(Root_FlattenUnflatten_ReturnsSame);
	CPPUNIT_TEST(Boot_FlattenUnflatten_ReturnsSame);
	CPPUNIT_TEST(Home_FlattenUnflatten_ReturnsSame);
	CPPUNIT_TEST(Desktop_FlattenUnflatten_ReturnsSame);
	CPPUNIT_TEST(NonExisting_FlattenUnflatten_ReturnsSame);
	CPPUNIT_TEST(BadArgs_Flattenable_ReturnsError);
	CPPUNIT_TEST_SUITE_END();

	void _FlattenUnflatten(const char* pathName)
	{
		BPath path;
		char buffer[1024];
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathName));
		BEntry entry(pathName);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		struct flattened_ref { dev_t device; ino_t directory; char name[1]; };
		ssize_t size = path.FlattenedSize();
		ssize_t expectedSize = sizeof(dev_t) + sizeof(ino_t) + strlen(ref.name) + 1;
		CPPUNIT_ASSERT(size == expectedSize);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Flatten(buffer, sizeof(buffer)));
		const flattened_ref &fref = *(flattened_ref*)buffer;
		CPPUNIT_ASSERT(ref.device == fref.device);
		CPPUNIT_ASSERT(ref.directory == fref.directory);
		CPPUNIT_ASSERT(strcmp(ref.name, fref.name) == 0);
		path.Unset();
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Unflatten(B_REF_TYPE, buffer, size));
		CPPUNIT_ASSERT(path == pathName);
	}

public:
	void setUp() { BasicTest::setUp(); }
	void tearDown() { BasicTest::tearDown(); }

	void DefaultConstructor_ReturnsNoInit()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Root_Constructor_ReturnsSame()
	{
		BPath path("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void Boot_Constructor_ReturnsSame()
	{
		BPath path("/boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbsoluteNonExistingNormalized_Constructor_ReturnsSame()
	{
		const char* pathName = "/doesn't/exist/but/who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (pathName) == path.Path());
	}

	void Root_Constructor_ReturnsNormalized()
	{
		BPath path("/.///.");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void Boot_Constructor_ReturnsNormalized()
	{
		BPath path("/boot/");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbsoluteNonExisting_Constructor_ReturnsNotFound()
	{
		const char* pathName = "/doesn't/exist/but///who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeExistingNormalized_Constructor_ReturnsSame()
	{
		chdir("/");
		const char* pathName = "boot";
		const char* absolutePathName = "/boot";
		BPath path(pathName);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (absolutePathName) == path.Path());
	}

	void RelativeNonExistingNormalized_Constructor_ReturnsNotFound()
	{
		chdir("/boot");
		const char* pathName = "doesn't/exist/but/who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeExisting_Constructor_ReturnsNormalized()
	{
		chdir("/");
		const char* pathName = "boot/";
		const char* normalizedPathName = "/boot";
		BPath path(pathName);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (normalizedPathName) == path.Path());
	}

	void RelativeNonExisting_Constructor_ReturnsNotFound()
	{
		chdir("/boot");
		const char* pathName = "doesn't/exist/but///who/cares";
		BPath path(pathName);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RootLeaf_Constructor_ReturnsSame()
	{
		BPath path("/", "boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void BootLeaf_Constructor_ReturnsSame()
	{
		BPath path("/boot", "home/Desktop");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void NonExistingLeaf_Constructor_ReturnsSame()
	{
		BPath path("/doesn't/exist", "but/who/cares");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/doesn't/exist/but/who/cares") == path.Path());
	}

	void RootLeaf_Constructor_ReturnsNormalized()
	{
		BPath path("/.///", ".");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void BootLeaf_Constructor_ReturnsNormalized()
	{
		BPath path("/boot", "home/..");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void NonExistingLeaf_Constructor_ReturnsNotFound()
	{
		BPath path("/doesn't/exist", "but//who/cares");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeExistingLeaf_Constructor_ReturnsNormalized()
	{
		chdir("/");
		BPath path("boot", "home");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home") == path.Path());
	}

	void RelativeNonExistingLeaf_Constructor_ReturnsNotFound()
	{
		chdir("/boot");
		BPath path("doesn't/exist", "but/who/cares");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeLeafWithTrailingSlash_Constructor_ReturnsNormalized()
	{
		chdir("/boot");
		BPath path("home", "Desktop//");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void AbsoluteLeaf_Constructor_ReturnsBadValue()
	{
		BPath path("/", "/boot");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NullPathAndLeaf_Constructor_ReturnsBadValue()
	{
		BPath path((const char*)NULL, "test");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NullPath_Constructor_ReturnsBadValue()
	{
		BPath path((const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Root_Constructor_ReturnsOk()
	{
		BDirectory dir("/");
		BPath path(&dir, NULL);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void Boot_Constructor_ReturnsOk()
	{
		BDirectory dir("/boot");
		BPath path(&dir, NULL);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void Entry_Constructor_ReturnsOk()
	{
		const char* pathName = "/";
		BEntry entry(pathName);
		BPath path(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (pathName) == path.Path());
	}

	void Ref_Constructor_ReturnsOk()
	{
		const char* pathName = "/";
		BEntry entry(pathName);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BPath path(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (pathName) == path.Path());
	}

	void RootForceNormalized_DirPathConstructor_ReturnsSame()
	{
		BDirectory dir("/");
		BPath path(&dir, NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void BootForceNormalized_DirPathConstructor_ReturnsSame()
	{
		BDirectory dir("/boot");
		BPath path(&dir, NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void RootLeaf_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir("/");
		BPath path(&dir, "boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void BootLeaf_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir("/boot");
		BPath path(&dir, "home/Desktop");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void LeafAutoNormalization_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir("/boot");
		BPath path(&dir, "home/..");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbsoluteLeaf_DirPathConstructor_ReturnsBadValue()
	{
		BDirectory dir("/");
		BPath path(&dir, "/boot");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void UninitializedDir_DirPathConstructor_ReturnsBadValue()
	{
		BDirectory dir;
		BPath path(&dir, "test");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void BadlyInitializedDir_DirPathConstructor_ReturnsBadValue()
	{
		BDirectory dir("/this/dir/doesn't/exists");
		BPath path(&dir, "test");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NullDirAndLeafName_DirPathConstructor_ReturnsBadValue()
	{
#if !TEST_R5
		// R5: crashs, when passing a NULL BDirectory
		BPath path((const BDirectory*)NULL, "test");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
#endif
	}

	void NullDirAndNullLeaf_DirPathConstructor_ReturnsBadValue()
	{
#if !TEST_R5
		// R5: crashs, when passing a NULL BDirectory
		BPath path((const BDirectory*)NULL, NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
#endif
	}

	void Boot_EntryConstructor_ReturnsOk()
	{
		BEntry entry("/boot");
		BPath path(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void Abstract_EntryConstructor_ReturnsOk()
	{
		const char* pathName = "/boot/shouldn't exist";
		BEntry entry(pathName);
		BPath path(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (pathName) == path.Path());
	}

	void UninitializedEntry_EntryConstructor_ReturnsNoInit()
	{
		BEntry entry;
		BPath path(&entry);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void BadlyInitializedEntry_EntryConstructor_ReturnsNoInit()
	{
		BEntry entry("/this/doesn't/exist");
		BPath path(&entry);
		CPPUNIT_ASSERT(equals(path.InitCheck(), B_NO_INIT, B_ENTRY_NOT_FOUND));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NullEntry_EntryConstructor_ReturnsBadValue()
	{
		BPath path((const BEntry*)NULL);
		CPPUNIT_ASSERT(equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Boot_RefConstructor_ReturnsOk()
	{
		BEntry entry("/boot");
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BPath path(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void Abstract_RefConstructor_ReturnsOk()
	{
		const char* pathName = "/boot/shouldn't exist";
		BEntry entry(pathName);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BPath path(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (pathName) == path.Path());
	}

	void NullRef_RefConstructor_ReturnsBadValue()
	{
		BPath path((entry_ref*)NULL);
		CPPUNIT_ASSERT(equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RootForceNormalized_Constructor_ReturnsSame()
	{
		BPath path("/", NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void BootForceNormalized_Constructor_ReturnsSame()
	{
		BPath path("/boot", NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void NonExistingForceNormalized_Constructor_ReturnsNotFound()
	{
		BPath path("/doesn't/exist/but/who/cares", NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeForceNormalized_Constructor_ReturnsOk()
	{
		chdir("/");
		BPath path("boot", NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void RelativeNonExistingForceNormalized_Constructor_ReturnsNotFound()
	{
		chdir("/boot");
		BPath path("doesn't/exist/but/who/cares", NULL, true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NonExistingLeafForceNormalized_Constructor_ReturnsNotFound()
	{
		BPath path("/doesn't/exist", "but/who/cares", true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Root_SetTo_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void Boot_SetTo_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbsoluteLeafNormalized_SetTo_ReturnsSame()
	{
		BPath path;
		const char* pathName = "/";
		const char* leafName = "boot";
		const char* absolutePathName = "/boot";
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathName, leafName));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (absolutePathName) == path.Path());
	}

	void AbsoluteExisting_SetTo_ReturnsNormalized()
	{
		BPath path;
		const char* pathName = "/.///.";
		const char* normalizedPathName = "/";
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathName));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (normalizedPathName) == path.Path());
	}

	void AbsoluteNonExisting_SetTo_ReturnsNotFound()
	{
		BPath path;
		const char* pathName = "/doesn't/exist/but///who/cares";
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.SetTo(pathName));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void AbsoluteNonExistingNormalized_SetTo_ReturnsSame()
	{
		BPath path;
		const char* pathName = "/doesn't/exist/but/who/cares";
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathName));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (pathName) == path.Path());
	}

	void RelativeExisting_SetTo_ReturnsNormalized()
	{
		BPath path;
		chdir("/");
		const char* pathName = "boot/";
		const char* normalizedPathName = "/boot";
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathName));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (normalizedPathName) == path.Path());
	}

	void RelativeNonExistingNormalized_SetTo_ReturnsNotFound()
	{
		BPath path;
		chdir("/boot");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.SetTo("doesn't/exist/but/who/cares"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeNonExistingLeafNormalized_SetTo_ReturnsNotFound()
	{
		BPath path;
		chdir("/boot");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.SetTo("doesn't/exist", "but/who/cares", true));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void SelfPath_SetTo_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(path.Path(), ".///./"));
		CPPUNIT_ASSERT(std::string ("/boot/home") == path.Path());
	}

	void ParentPath_SetTo_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(path.Path(), ".."));
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void LeafPath_SetTo_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(path.Path(), "home/Desktop"));
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void BadArgs_SetTo_ReturnsBadValue()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo("/", "/boot"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void BootLeaf_SetTo_ReturnsOk()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot", "home/Desktop"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void NonNormalized_NonExistingLeaf_SetTo_ReturnsOk()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/doesn't/exist", "but/who/cares"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(
			std::string ("/doesn't/exist/but/who/cares") == path.Path());
	}

	void RootDotLeaf_SetTo_ReturnsNormalized()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/.///", "."));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void NonExistingLeafAutoNormalization_SetTo_ReturnsNotFound()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.SetTo("/doesn't/exist", "but//who/cares"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NonExistingLeafForceNormalized_SetTo_ReturnsNotFound()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.SetTo("/doesn't/exist", "but/who/cares", true));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeNonExistingLeaf_SetTo_ReturnsNotFound()
	{
		BPath path;
		chdir("/boot");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.SetTo("doesn't/exist", "but/who/cares"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeAutoNormalizationLeaf_SetTo_ReturnsNormalized()
	{
		BPath path;
		chdir("/boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("home", "Desktop//"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void ForceNormalized_SetTo_ReturnsOk()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/", NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot", NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void NonExistingForceNormalized_SetTo_ReturnsNotFound()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND,
			path.SetTo("/doesn't/exist/but/who/cares", NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RelativeForceNormalized_SetTo_ReturnsOk()
	{
		BPath path;
		chdir("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("boot", NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void RelativeNonExistingForceNormalized_SetTo_ReturnsNotFound()
	{
		BPath path;
		chdir("/boot");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND,
			path.SetTo("doesn't/exist/but/who/cares", NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NullArgs_SetTo_ReturnsBadValue()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo((const char*)NULL, "test"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo((const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void RootForceNormalized_SetToDirPath_ReturnsSame()
	{
		BDirectory dir("/");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&dir, NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void BootForceNormalized_SetToDirPath_ReturnsSame()
	{
		BDirectory dir("/boot");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&dir, NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void RootLeaf_SetToDirPath_ReturnsOk()
	{
		BDirectory dir("/");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&dir, "boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void BootLeaf_SetToDirPath_ReturnsOk()
	{
		BDirectory dir("/boot");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&dir, "home/Desktop"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
	}

	void BootAutoNormalizationLeaf_SetToDirPath_ReturnsOk()
	{
		BDirectory dir("/boot");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&dir, "home/.."));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void DirAbsoluteLeaf_SetTo_ReturnsBadValue()
	{
		BDirectory dir("/");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo(&dir, "/boot"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void DirUninitialized_SetTo_ReturnsBadValue()
	{
		BDirectory dir;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo(&dir, "test"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void DirBadlyInitialized_SetTo_ReturnsBadValue()
	{
		BDirectory dir("/this/dir/doesn't/exists");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo("/this/dir/doesn't/exists"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo(&dir, "test"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void DirNull_SetTo_ReturnsBadValue()
	{
#if !TEST_R5
		// R5: crashs, when passing a NULL BDirectory
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo((const BDirectory*)NULL, "test"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.SetTo((const BDirectory*)NULL, NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
#endif
	}

	void Root_SetToEntry_ReturnsOk()
	{
		BEntry entry("/");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void Boot_SetToEntry_ReturnsOk()
	{
		BEntry entry("/boot");
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbstractEntry_SetTo_ReturnsOk()
	{
		const char* abstractName = "/boot/shouldn't exist";
		BEntry entry(abstractName);
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(abstractName));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (abstractName) == path.Path());
	}

	void EntryUninitialized_SetTo_ReturnsNoInit()
	{
		BEntry entry;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void EntryBadlyInitialized_SetTo_ReturnsNoInit()
	{
		BEntry entry;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo("/this/doesn't/exist"));
		CPPUNIT_ASSERT(
			equals(path.SetTo(&entry), B_NO_INIT, B_ENTRY_NOT_FOUND));
		CPPUNIT_ASSERT(
			equals(path.InitCheck(), B_NO_INIT, B_ENTRY_NOT_FOUND));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void EntryNull_SetTo_ReturnsBadValue()
	{
		BPath path;
		CPPUNIT_ASSERT(
			equals(path.SetTo((const BEntry*)NULL), B_NO_INIT, B_BAD_VALUE));
		CPPUNIT_ASSERT(
			equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Root_SetToRef_ReturnsOk()
	{
		BEntry entry("/");
		entry_ref ref;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
	}

	void Boot_SetToRef_ReturnsOk()
	{
		BEntry entry("/boot");
		entry_ref ref;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbstractEntry_SetToRef_ReturnsOk()
	{
		const char* abstractName = "/boot/shouldn't exist";
		BEntry entry(abstractName);
		entry_ref ref;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(abstractName));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string (abstractName) == path.Path());
	}

	void RefNull_SetTo_ReturnsBadValue()
	{
		BPath path;
		CPPUNIT_ASSERT(
			equals(path.SetTo((const entry_ref*)NULL), B_NO_INIT, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(path.InitCheck(), B_NO_INIT, B_BAD_VALUE));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Uninitialized_Append_ReturnsBadValue()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.Append("test"));
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void Hierarchy_Append_ReturnsExpected()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/"));
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Append("boot"));
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Append("home/Desktop"));
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Append("non/existing"));
		CPPUNIT_ASSERT(std::string ("/boot/home/Desktop/non/existing") == path.Path());

		// trigger normalization
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.Append("at/least/not//now"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void ForceNormalization_Append_ReturnsExpected()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Append("home/non-existing", true));
		CPPUNIT_ASSERT(std::string ("/boot/home/non-existing") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.Append("not/now", true));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.InitCheck());
		CPPUNIT_ASSERT(path.Path() == NULL);
	}

	void NullLeaf_Append_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Append(NULL));
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void AbsoluteLeaf_Append_ReturnsBadValue()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.Append("/tmp"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.InitCheck());
	}

	void EmptyLeaf_Append_ReturnsSame()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Append(""));
		CPPUNIT_ASSERT(std::string ("/boot") == path.Path());
	}

	void Uninitialized_Leaf_ReturnsNull()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
		CPPUNIT_ASSERT(path.Leaf() == NULL);
	}

	void Root_Leaf_ReturnsEmpty()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/"));
		CPPUNIT_ASSERT(std::string ("") == path.Leaf());
	}

	void ExistingBoot_Leaf_ReturnsName()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT(std::string ("boot") == path.Leaf());
	}

	void ExistingHome_Leaf_ReturnsName()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home"));
		CPPUNIT_ASSERT(std::string ("home") == path.Leaf());
	}

	void ExistingDesktop_Leaf_ReturnsName()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home/Desktop"));
		CPPUNIT_ASSERT(std::string ("Desktop") == path.Leaf());
	}

	void NonExistingPath_Leaf_ReturnsName()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/non-existing"));
		CPPUNIT_ASSERT(std::string ("non-existing") == path.Leaf());
	}

	void NonExistingDirectory_Leaf_ReturnsName()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/non/existing/dir"));
		CPPUNIT_ASSERT(std::string ("dir") == path.Leaf());
	}

	void Uninitialized_GetParent_ReturnsNoInit()
	{
		BPath path;
		BPath parent;
#if !TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.GetParent(&parent));
#endif
	}

	void Root_GetParent_ReturnsNotFound()
	{
		BPath path;
		BPath parent;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.GetParent(&parent));
	}

	void Boot_GetParent_ReturnsRoot()
	{
		BPath path;
		BPath parent;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&parent));
		CPPUNIT_ASSERT(std::string ("/") == parent.Path());
	}

	void Home_GetParent_ReturnsBoot()
	{
		BPath path;
		BPath parent;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&parent));
		CPPUNIT_ASSERT(std::string ("/boot") == parent.Path());
	}

	void Desktop_GetParent_ReturnsHome()
	{
		BPath path;
		BPath parent;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/boot/home/Desktop"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&parent));
		CPPUNIT_ASSERT(std::string ("/boot/home") == parent.Path());
	}

	void NonExisting_GetParent_ReturnsRoot()
	{
		BPath path;
		BPath parent;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/non-existing"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&parent));
		CPPUNIT_ASSERT(std::string ("/") == parent.Path());
	}

	void NonExistingDirectory_GetParent_ReturnsParent()
	{
		BPath path;
		BPath parent;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/non/existing/dir"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&parent));
		CPPUNIT_ASSERT(std::string ("/non/existing") == parent.Path());
	}

	void Sequence_GetParent_ReturnsExpected()
	{
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/non/existing/dir"));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&path));
		CPPUNIT_ASSERT(std::string ("/non/existing") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&path));
		CPPUNIT_ASSERT(std::string ("/non") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_OK, path.GetParent(&path));
		CPPUNIT_ASSERT(std::string ("/") == path.Path());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, path.GetParent(&path));
	}

	void NullArg_GetParent_ReturnsBadValue()
	{
#if !TEST_R5
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/non/existing/dir"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.GetParent(NULL));
#endif
	}

	void Uninitialized_BPathComparison_ReturnsEqual()
	{
#if !TEST_R5
		BPath path;
		BPath path2;
		CPPUNIT_ASSERT((path == path2) == true);
		CPPUNIT_ASSERT((path != path2) == false);
#endif
	}

	void Initialized_BPathComparison_ReturnsDifferent()
	{
		BPath path;
		BPath path2;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo("/"));
		CPPUNIT_ASSERT((path == path2) == false);
		CPPUNIT_ASSERT((path != path2) == true);
		path.Unset();
		CPPUNIT_ASSERT_EQUAL(B_OK, path2.SetTo("/"));
		CPPUNIT_ASSERT((path == path2) == false);
		CPPUNIT_ASSERT((path != path2) == true);
	}

	void PathNames_BPathComparison_ReturnsExpected()
	{
		BPath path;
		BPath path2;
		const char* pathNames[] = {"/", "/boot", "/boot/home", "/boot/home/Desktop"};
		int32 pathCount = sizeof(pathNames) / sizeof(const char*);
		for (int32 i = 0; i < pathCount; i++) {
			for (int32 k = 0; k < pathCount; k++) {
				CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathNames[i]));
				CPPUNIT_ASSERT_EQUAL(B_OK, path2.SetTo(pathNames[k]));
				CPPUNIT_ASSERT((path == path2) == (i == k));
				CPPUNIT_ASSERT((path != path2) == (i != k));
			}
		}
	}

	void Uninitialized_CharPtrComparison_ReturnsExpected()
	{
		BPath path;
		const char* pathName = "/";
		CPPUNIT_ASSERT((path == pathName) == false);
		CPPUNIT_ASSERT((path != pathName) == true);
	}

	void PathNames_CharPtrComparison_ReturnsExpected()
	{
		BPath path;
		const char* pathNames[] = {"/", "/boot", "/boot/home", "/boot/home/Desktop"};
		int32 pathCount = sizeof(pathNames) / sizeof(const char*);
		for (int32 i = 0; i < pathCount; i++) {
			for (int32 k = 0; k < pathCount; k++) {
				CPPUNIT_ASSERT_EQUAL(B_OK, path.SetTo(pathNames[i]));
				CPPUNIT_ASSERT((path == pathNames[k]) == (i == k));
				CPPUNIT_ASSERT((path != pathNames[k]) == (i != k));
			}
		}
	}

	void Null_CharPtrComparison_ReturnsExpected()
	{
#if !TEST_R5
		BPath path;
		path.SetTo("/");
		CPPUNIT_ASSERT((path == (const char*)NULL) == false);
		CPPUNIT_ASSERT((path != (const char*)NULL) == true);
		path.Unset();
		CPPUNIT_ASSERT((path == (const char*)NULL) == true);
		CPPUNIT_ASSERT((path != (const char*)NULL) == false);
#endif
	}

	void Uninitialized_CopyConstructor_ReturnsNoInit()
	{
		BPath path;
		BPath path2(path);
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path2.InitCheck());
	}

	void Initialized_CopyConstructor_ReturnsCopy()
	{
		BPath path("/boot/home/Desktop");
		BPath path2(path);
		CPPUNIT_ASSERT_EQUAL(B_OK, path2.InitCheck());
		CPPUNIT_ASSERT(path == path2);
	}

	void Uninitialized_BPathAssignment_ReturnsNoInit()
	{
		BPath path;
		BPath path2;
		path2 = path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path2.InitCheck());
	}

	void InitializedToUninitialized_BPathAssignment_ReturnsNoInit()
	{
		BPath path;
		BPath path2("/");
		path2 = path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path2.InitCheck());
	}

	void Initialized_BPathAssignment_ReturnsCopy()
	{
		BPath path("/boot/home");
		BPath path2;
		path2 = path;
		CPPUNIT_ASSERT_EQUAL(B_OK, path2.InitCheck());
		CPPUNIT_ASSERT(path == path2);
	}

	void CharPtr_Assignment_ReturnsPath()
	{
		const char* pathName = "/boot/home";
		BPath path;
		path = pathName;
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(path == pathName);
	}

	void NullCharPtr_Assignment_ReturnsNoInit()
	{
		BPath path;
		path = (const char*)NULL;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
	}

	void Trivial_Flattenable_ReturnsExpected()
	{
		BPath path;
		CPPUNIT_ASSERT(path.IsFixedSize() == false);
		CPPUNIT_ASSERT(path.TypeCode() == B_REF_TYPE);
		CPPUNIT_ASSERT(path.AllowsTypeCode(B_REF_TYPE) == true);
		CPPUNIT_ASSERT(path.AllowsTypeCode(B_STRING_TYPE) == false);
		CPPUNIT_ASSERT(path.AllowsTypeCode(B_FLOAT_TYPE) == false);

		path.SetTo("/boot/home");
		CPPUNIT_ASSERT(path.IsFixedSize() == false);
		CPPUNIT_ASSERT(path.TypeCode() == B_REF_TYPE);
		CPPUNIT_ASSERT(path.AllowsTypeCode(B_REF_TYPE) == true);
		CPPUNIT_ASSERT(path.AllowsTypeCode(B_STRING_TYPE) == false);
		CPPUNIT_ASSERT(path.AllowsTypeCode(B_FLOAT_TYPE) == false);
	}

	void Uninitialized_FlattenUnflatten_ReturnsNoInit()
	{
		BPath path;
		char buffer[1024];
		ssize_t size = path.FlattenedSize();
		CPPUNIT_ASSERT(size == sizeof(dev_t) + sizeof(ino_t));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Flatten(buffer, sizeof(buffer)));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.Unflatten(B_REF_TYPE, buffer, size));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, path.InitCheck());
	}

	void Root_FlattenUnflatten_ReturnsSame() { _FlattenUnflatten("/"); }

	void Boot_FlattenUnflatten_ReturnsSame() { _FlattenUnflatten("/boot"); }

	void Home_FlattenUnflatten_ReturnsSame() { _FlattenUnflatten("/boot/home"); }

	void Desktop_FlattenUnflatten_ReturnsSame() { _FlattenUnflatten("/boot/home/Desktop"); }

	void NonExisting_FlattenUnflatten_ReturnsSame()
	{
		_FlattenUnflatten("/boot/home/non-existing");
	}

	void BadArgs_Flattenable_ReturnsError()
	{
#if !TEST_R5
		BPath path("/boot/home");
		char buffer[1024];
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.Flatten(NULL, sizeof(buffer)));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, path.Flatten(buffer, path.FlattenedSize() - 2));
#endif
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(PathTest, getTestSuiteName());
