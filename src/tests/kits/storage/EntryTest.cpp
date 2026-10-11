/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>
#include <map>
#include <set>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Node.h>
#include <Path.h>
#include <SymLink.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "StatableTest.h"


struct TestEntry;
static status_t get_entry_ref_for_entry(const char* dir, const char* leaf,
	entry_ref* ref) 
{
	status_t error = (dir && leaf ? B_OK : B_BAD_VALUE);
	struct stat dirStat;
	if (lstat(dir, &dirStat) == 0) {
		ref->device = dirStat.st_dev;
		ref->directory = dirStat.st_ino;
		ref->set_name(leaf);
	} else
		error = errno;
	return error;
}
static std::string get_shortest_relative_path(TestEntry* dir, TestEntry* entry);

static inline bool
operator>(const entry_ref& a, const entry_ref& b)
{
	return (a.device > b.device
		|| (a.device == b.device
			&& (a.directory > b.directory
			|| (a.directory == b.directory
				&& (a.name != NULL && b.name == NULL
				|| (a.name != NULL && b.name != NULL
					&& strcmp(a.name, b.name) > 0))))));
}

enum test_entry_kind {
	DIR_ENTRY,
	FILE_ENTRY,
	LINK_ENTRY,
	ABSTRACT_ENTRY,
	BAD_ENTRY,
};

static std::vector<TestEntry*> allTestEntries;

struct TestEntry {
	TestEntry();

	bool isConcrete() const { return !(isAbstract() || isBad()); }

	bool isAbstract() const { return kind == ABSTRACT_ENTRY; }

	bool isBad() const { return kind == BAD_ENTRY; }

	TestEntry *		super;
	std::string		name;
	test_entry_kind	kind;
	std::string		path;
	TestEntry *		target;
	std::string		link;
	entry_ref		ref;
	bool			relative;
	// C strings
	const char*		cname;
	const char*		cpath;
	const char*		clink;

	const entry_ref& get_ref()
	{
		if (isConcrete() || isAbstract())
			get_entry_ref_for_entry(super->cpath, cname, &ref);
		return ref;
	}

	void completeInit()
	{
		// init link
		if (kind == LINK_ENTRY) {
			if (relative)
				link = get_shortest_relative_path(super, target);
			else
				link = target->path;
		}
		// init the C strings
		cname = name.c_str();
		cpath = path.c_str();
		clink = link.c_str();
	}

	void initPath(const char* pathName = NULL)
	{
		if (pathName)
			path = pathName;
		else
			path = super->path + "/" + name;
	}

	void initALink(TestEntry& super, std::string name, TestEntry& target)
	{
		init(super, name, LINK_ENTRY, false);
		this->target = &target;
	}

	void initRLink(TestEntry& super, std::string name, TestEntry& target)
	{
		init(super, name, LINK_ENTRY, true);
		this->target = &target;
	}

	void initFile(TestEntry& super, std::string name) { init(super, name, FILE_ENTRY); }

	void initDir(TestEntry& super, std::string name) { init(super, name, DIR_ENTRY); }

	void init(TestEntry& super, std::string name, test_entry_kind kind, bool relative = false)
	{
		this->super = &super;
		this->name = name;
		this->kind = kind;
		this->relative = relative;
		allTestEntries.push_back(this);
	}
};

static TestEntry badTestEntry;


static void
compareEntries(BEntry& entry, BEntry& entry2, TestEntry* testEntry, TestEntry* testEntry2,
	bool traversed, bool traversed2)
{
	if (!testEntry->isBad() && !testEntry2->isBad()
		&& (!traversed && testEntry->isConcrete())
		&& (!traversed2 && testEntry2->isConcrete())) {
		CPPUNIT_ASSERT((entry == entry2) == (testEntry == testEntry2));
		CPPUNIT_ASSERT((entry == entry2) == (testEntry2 == testEntry));
		CPPUNIT_ASSERT((entry != entry2) == (testEntry != testEntry2));
		CPPUNIT_ASSERT((entry != entry2) == (testEntry2 != testEntry));
	}
}

static std::string setUpCommandLine;
static std::string tearDownCommandLine;

// forward declarations
static TestEntry* resolve_link(TestEntry* entry);
static std::string get_shortest_relative_path(TestEntry* dir, TestEntry* entry);

static const status_t kErrors[] = {
	B_BAD_ADDRESS,
	B_BAD_VALUE,
	B_CROSS_DEVICE_LINK,
	B_DEVICE_FULL,
	B_DIRECTORY_NOT_EMPTY,
	B_ENTRY_NOT_FOUND,
	B_ERROR,
	B_FILE_ERROR,
	B_FILE_EXISTS,
	B_IS_A_DIRECTORY,
	B_LINK_LIMIT,
	B_NAME_TOO_LONG,
	B_NO_MORE_FDS,
	B_NOT_A_DIRECTORY,
	B_OK,
	B_PARTITION_TOO_SMALL,
	B_READ_ONLY_DEVICE,
	B_UNSUPPORTED,
	E2BIG
};
static const int32 kErrorCount = sizeof(kErrors) / sizeof(status_t);

static int32
get_error_index(status_t error)
{
	int32 result = -1;
	for (int32 i = 0; result == -1 && i < kErrorCount; i++) {
		if (kErrors[i] == error)
			result = i;
	}

	if (result == -1)
		printf("WARNING: error %lx is not in the list of errors\n", (unsigned long)error);
	return result;
}

static status_t
fuzzy_error(status_t error1, status_t error2)
{
	status_t result = error1;
	// encode the two errors in one value
	int32 index1 = get_error_index(error1);
	int32 index2 = get_error_index(error2);
	if (index1 >= 0 && index2 >= 0)
		result = index1 * kErrorCount + index2 + 1;
	return result;
}

static bool
fuzzy_equals(status_t error, status_t fuzzyError)
{
	bool result = false;
	if (fuzzyError <= 0) {
		result = (error == fuzzyError);
	} else {
		// decode the error
		int32 index1 = (fuzzyError - 1) / kErrorCount;
		int32 index2 = (fuzzyError - 1) % kErrorCount;
		if (index1 >= kErrorCount) {
			printf("WARNING: bad fuzzy error: %lx\n", (unsigned long)fuzzyError);
		} else {
			status_t error1 = kErrors[index1];
			status_t error2 = kErrors[index2];
			result = (error == error1 || error == error2);
		}
	}
	return result;
}


static void
examine_entry(BEntry& entry, TestEntry* testEntry, bool traverse)
{
	if (traverse)
		testEntry = resolve_link(testEntry);
	// Exists()
	CPPUNIT_ASSERT(entry.Exists() == testEntry->isConcrete());
	// GetPath()
	BPath path;
	CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
	CPPUNIT_ASSERT(path == testEntry->cpath);
	// GetName()
	char name[B_FILE_NAME_LENGTH + 1];
	CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetName(name));
	CPPUNIT_ASSERT(testEntry->name == name);
	// GetParent(BEntry *)
	BEntry parentEntry;
	CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetParent(&parentEntry));
	CPPUNIT_ASSERT_EQUAL(B_OK, parentEntry.InitCheck());
	CPPUNIT_ASSERT_EQUAL(B_OK, parentEntry.GetPath(&path));
	CPPUNIT_ASSERT(path == testEntry->super->cpath);
	parentEntry.Unset();
	path.Unset();
	// GetParent(BDirectory *)
	BDirectory parentDir;
	CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetParent(&parentDir));
	CPPUNIT_ASSERT_EQUAL(B_OK, parentDir.GetEntry(&parentEntry));
	CPPUNIT_ASSERT_EQUAL(B_OK, parentEntry.InitCheck());
	CPPUNIT_ASSERT_EQUAL(B_OK, parentEntry.GetPath(&path));
	CPPUNIT_ASSERT(path == testEntry->super->cpath);
	// GetRef()
	entry_ref ref;
	CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
	// We can't get a ref of an entry with a too long path name yet.
	if (testEntry->path.length() < B_PATH_NAME_LENGTH)
		CPPUNIT_ASSERT(ref == testEntry->get_ref());
}

bool
isHarmlessPathname(const char* path)
{
	bool result = false;
	if (path) {
		const char* harmlessDir = "/tmp/";
		result = (std::string (path).find(harmlessDir) == 0);
	}
	if (!result)
		printf("WARNING: `%s' is not a harmless pathname.\n", path);
	return result;
}

static const char* tooLongDirname
	= "1234567890123456789012345678901234567890123456789012345678901234567890";

// too long entry name (257 characters)
static const char* tooLongEntryname
	= "1234567890123456789012345678901234567890123456789012345678901234567890"
	  "1234567890123456789012345678901234567890123456789012345678901234567890"
	  "1234567890123456789012345678901234567890123456789012345678901234567890"
	  "12345678901234567890123456789012345678901234567";

// TestEntry

// constructor
TestEntry::TestEntry()
	: super(&badTestEntry),
	  name("badTestEntry"),
	  kind(BAD_ENTRY),
	  path("/tmp/badTestEntry"),
	  target(&badTestEntry),
	  link(),
	  ref(),
	  relative(true),
	  cname(""),
	  cpath(""),
	  clink("")
{
}

static string
get_shortest_relative_path(TestEntry* dir, TestEntry* entry)
{
	std::string relPath;
	// put all super directories (including dir itself) of the dir in a set
	std::map<TestEntry*, int> superDirs;
	int dirSuperLevel = 0;
	for (TestEntry* superDir = dir; superDir != &badTestEntry;
		superDir = superDir->super, dirSuperLevel++) {
		superDirs[superDir] = dirSuperLevel;
	}
	// find the first super dir that dir and entry have in common
	TestEntry* commonSuperDir = &badTestEntry;
	int targetSuperLevel = 0;
	for (TestEntry* superDir = entry; commonSuperDir == &badTestEntry && superDir != &badTestEntry;
		superDir = superDir->super, targetSuperLevel++) {
		if (superDirs.find(superDir) != superDirs.end())
			commonSuperDir = superDir;
	}
	// construct the relative path
	if (commonSuperDir != &badTestEntry) {
		dirSuperLevel = superDirs[commonSuperDir];
		if (dirSuperLevel == 0 && targetSuperLevel == 0) {
			// entry == dir
			relPath = ".";
		} else {
			// levels down
			for (TestEntry* superDir = entry; superDir != commonSuperDir;
				superDir = superDir->super) {
				if (relPath.length() == 0)
					relPath = superDir->name;
				else
					relPath = superDir->name + "/" + relPath;
			}
			// levels up
			for (int i = dirSuperLevel; i > 0; i--)
				if (relPath.length() == 0)
					relPath = "..";
				else
					relPath = std::string ("../") + relPath;
		}
	}
	return relPath;
}

static TestEntry*
resolve_link(TestEntry* entry)
{
	std::set<TestEntry*> followedLinks;
	while (entry != &badTestEntry && entry->kind == LINK_ENTRY) {
		if (followedLinks.find(entry) == followedLinks.end()) {
			followedLinks.insert(entry);
			entry = entry->target;
		} else {
			entry = &badTestEntry; // cyclic link
		}
	}
	return entry;
}


class BEntry;
struct TestEntry;

class EntryTest : public StatableTest {
	CPPUNIT_TEST_SUB_SUITE(EntryTest, StatableTest);
	CPPUNIT_TEST(DefaultConstructor_ReturnsNoInit);
	CPPUNIT_TEST(Existing_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(Link_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(BadLink_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(CyclicLink_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(TooLong_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(Root_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(Boot_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(Null_PathConstructor_ReturnsBadValue);

	CPPUNIT_TEST(Existing_RefConstructor_ReturnsExpected);
	CPPUNIT_TEST(Link_RefConstructor_ReturnsExpected);
	CPPUNIT_TEST(BadLink_RefConstructor_ReturnsExpected);
	CPPUNIT_TEST(CyclicLink_RefConstructor_ReturnsExpected);
	CPPUNIT_TEST(Null_RefConstructor_ReturnsExpected);

	CPPUNIT_TEST(Existing_DirPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(Link_DirPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(BadLink_DirPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(CyclicLink_DirPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(TooLong_DirPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(RootDotLeaf_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(RootBootApplicationLeaf_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(NullLeaf_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(NullDirAndValidLeaf_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndNullLeaf_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndAbsolutePath_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(BadlyInitializedDir_DirPathConstructor_ReturnsOk);

	CPPUNIT_TEST(Existing_SetToPath_ReturnsExpected);
	CPPUNIT_TEST(Link_SetToPath_ReturnsExpected);
	CPPUNIT_TEST(BadLink_SetToPath_ReturnsExpected);
	CPPUNIT_TEST(CyclicLink_SetToPath_ReturnsExpected);
	CPPUNIT_TEST(TooLong_SetToPath_ReturnsExpected);
	CPPUNIT_TEST(Root_SetToPath_ReturnsOk);
	CPPUNIT_TEST(Boot_SetToPath_ReturnsOk);
	CPPUNIT_TEST(Null_SetToPath_ReturnsBadValue);

	CPPUNIT_TEST(Existing_SetToRef_ReturnsExpected);
	CPPUNIT_TEST(Link_SetToRef_ReturnsExpected);
	CPPUNIT_TEST(BadLink_SetToRef_ReturnsExpected);
	CPPUNIT_TEST(CyclicLink_SetToRef_ReturnsExpected);
	CPPUNIT_TEST(Null_SetToRef_ReturnsExpected);

	CPPUNIT_TEST(Existing_SetToDirPath_ReturnsExpected);
	CPPUNIT_TEST(Link_SetToDirPath_ReturnsExpected);
	CPPUNIT_TEST(BadLink_SetToDirPath_ReturnsExpected);
	CPPUNIT_TEST(CyclicLink_SetToDirPath_ReturnsExpected);
	CPPUNIT_TEST(TooLong_SetToDirPath_ReturnsExpected);
	CPPUNIT_TEST(RootDotLeaf_SetToDirPath_ReturnsOk);
	CPPUNIT_TEST(RootBootApplicationLeaf_SetToDirPath_ReturnsOk);
	CPPUNIT_TEST(NullLeaf_SetToDirPath_ReturnsOk);
	CPPUNIT_TEST(NullDirAndLeafName_SetToDirPath_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndNullLeaf_SetToDirPath_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndAbsolutePath_SetToDirPath_ReturnsBadValue);
	CPPUNIT_TEST(BadlyInitializedDir_SetToDirPath_ReturnsOk);

	CPPUNIT_TEST(Initialized_Unset_InitCheckReturnsNoInit);

	CPPUNIT_TEST(Uninitialized_Exists_ReturnsFalse);
	CPPUNIT_TEST(BadlyInitialized_Exists_ReturnsFalse);
	CPPUNIT_TEST(Root_Exists_ReturnsTrue);

	CPPUNIT_TEST(Uninitialized_GetPath_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitialized_GetPath_ReturnsNoInit);
	CPPUNIT_TEST(TooLongPath_GetPath_ReturnsOk);

	CPPUNIT_TEST(Uninitialized_GetName_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitialized_GetName_ReturnsNoInit);

	CPPUNIT_TEST(Uninitialized_GetParentEntry_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitialized_GetParentEntry_ReturnsNoInit);
	CPPUNIT_TEST(Root_GetParentEntry_ReturnsEntryNotFound);

	CPPUNIT_TEST(Uninitialized_GetParentDir_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitialized_GetParentDir_ReturnsNoInit);
	CPPUNIT_TEST(Root_GetParentDir_ReturnsEntryNotFound);

	CPPUNIT_TEST(Uninitialized_GetRef_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitialized_GetRef_ReturnsNoInit);
	CPPUNIT_TEST(Root_GetRef_ReturnsOk);

	CPPUNIT_TEST(SameDir_RenameOnFile_ReturnsExpected);
	CPPUNIT_TEST(DifferentDir_RenameOnFile_ReturnsExpected);
	CPPUNIT_TEST(Clobber_RenameOnFile_ReturnsExpected);
	CPPUNIT_TEST(NoClobber_RenameOnFile_ReturnsExpected);
	CPPUNIT_TEST(SameDir_RenameOnDir_ReturnsExpected);
	CPPUNIT_TEST(DifferentDir_RenameOnDir_ReturnsExpected);
	CPPUNIT_TEST(Clobber_RenameOnDir_ReturnsExpected);
	CPPUNIT_TEST(NoClobber_RenameOnDir_ReturnsExpected);
	CPPUNIT_TEST(SameDir_RenameOnLink_ReturnsExpected);
	CPPUNIT_TEST(DifferentDir_RenameOnLink_ReturnsExpected);
	CPPUNIT_TEST(Clobber_RenameOnLink_ReturnsExpected);
	CPPUNIT_TEST(NoClobber_RenameOnLink_ReturnsExpected);
	CPPUNIT_TEST(File_Rename_DoesNotClobberNotEmptyDir);
	CPPUNIT_TEST(File_Rename_ClobbersEmptyDir);
	CPPUNIT_TEST(Abstract_Rename_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_Rename_ReturnsExpected);
	CPPUNIT_TEST(Root_Rename_Fails);
	CPPUNIT_TEST(Abstract_Rename_Fails);
	CPPUNIT_TEST(Null_Rename_Fails);

	CPPUNIT_TEST(NullPath_MoveToFile_ReturnsExpected);
	CPPUNIT_TEST(WithPath_MoveToFile_ReturnsExpected);
	CPPUNIT_TEST(NullPath_MoveToDir_ReturnsExpected);
	CPPUNIT_TEST(WithPath_MoveToDir_ReturnsExpected);
	CPPUNIT_TEST(NullPath_MoveToLink_ReturnsExpected);
	CPPUNIT_TEST(WithPath_MoveToLink_ReturnsExpected);
	CPPUNIT_TEST(File_MoveTo_DoesNotClobberNotEmptyDir);
	CPPUNIT_TEST(File_MoveTo_ClobbersEmptyDir);
	CPPUNIT_TEST(Abstract_MoveTo_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_MoveTo_ReturnsExpected);
	CPPUNIT_TEST(BadInit_MoveTo_ReturnsExpected);
	CPPUNIT_TEST(NullDir_MoveTo_ReturnsBadValue);
	CPPUNIT_TEST(UninitializedDir_MoveTo_ReturnsBadValue);

	CPPUNIT_TEST(File_Remove_ReturnsExpected);
	CPPUNIT_TEST(EmptyDir_Remove_ReturnsOk);
	CPPUNIT_TEST(NonEmptyDir_Remove_ReturnsDirectoryNotEmpty);
	CPPUNIT_TEST(Link_Remove_ReturnsExpected);
	CPPUNIT_TEST(Abstract_Remove_ReturnsEntryNotFound);
	CPPUNIT_TEST(Uninitialized_Remove_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitialized_Remove_ReturnsNoInit);

	CPPUNIT_TEST(Uninitialized_Comparison_ReturnsExpected);
	CPPUNIT_TEST(InitializedAndUninitialized_Comparison_ReturnsExpected);
	CPPUNIT_TEST(UninitializedAndInitialized_Comparison_ReturnsExpected);
	CPPUNIT_TEST(Initialized_Comparison_ReturnsExpected);

	CPPUNIT_TEST(Uninitialized_CopyConstructor_ReturnsExpected);
	CPPUNIT_TEST(Initialized_CopyConstructor_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_Assignment_ReturnsExpected);
	CPPUNIT_TEST(Initialized_Assignment_ReturnsExpected);
	CPPUNIT_TEST(InitializedAndUninitialized_Assignment_ReturnsExpected);

	CPPUNIT_TEST(LockedNode_Exists_ReturnsTrue);

	CPPUNIT_TEST(EntryRef_GetRefForPath_ReturnsExpected);
	CPPUNIT_TEST(Roots_GetRefForPath_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_GetRefForPath_ReturnsExpected);
	CPPUNIT_TEST(EntryRef_GetRefForPath_ComparisonsReturnExpected);

	CPPUNIT_TEST_SUITE_END();

	TestEntry testDir;
	TestEntry dir1;
	TestEntry dir2;
	TestEntry file1;
	TestEntry file2;
	TestEntry file3;
	TestEntry file4;
	TestEntry subDir1;
	TestEntry subDir2;
	TestEntry subDir3;
	TestEntry abstractEntry1;
	TestEntry badEntry1;
	TestEntry absDirLink1;
	TestEntry absDirLink2;
	TestEntry absDirLink3;
	TestEntry absDirLink4;
	TestEntry relDirLink1;
	TestEntry relDirLink2;
	TestEntry relDirLink3;
	TestEntry relDirLink4;
	TestEntry absFileLink1;
	TestEntry absFileLink2;
	TestEntry absFileLink3;
	TestEntry absFileLink4;
	TestEntry relFileLink1;
	TestEntry relFileLink2;
	TestEntry relFileLink3;
	TestEntry relFileLink4;
	TestEntry absCyclicLink1;
	TestEntry absCyclicLink2;
	TestEntry relCyclicLink1;
	TestEntry relCyclicLink2;
	TestEntry absBadLink1;
	TestEntry absBadLink2;
	TestEntry absBadLink3;
	TestEntry absBadLink4;
	TestEntry relBadLink1;
	TestEntry relBadLink2;
	TestEntry relBadLink3;
	TestEntry relBadLink4;
	TestEntry absVeryBadLink1;
	TestEntry absVeryBadLink2;
	TestEntry absVeryBadLink3;
	TestEntry absVeryBadLink4;
	TestEntry relVeryBadLink1;
	TestEntry relVeryBadLink2;
	TestEntry relVeryBadLink3;
	TestEntry relVeryBadLink4;
	TestEntry tooLongEntry1;
	TestEntry tooLongDir1;
	TestEntry tooLongDir2;
	TestEntry tooLongDir3;
	TestEntry tooLongDir4;
	TestEntry tooLongDir5;
	TestEntry tooLongDir6;
	TestEntry tooLongDir7;
	TestEntry tooLongDir8;
	TestEntry tooLongDir9;
	TestEntry tooLongDir10;
	TestEntry tooLongDir11;
	TestEntry tooLongDir12;
	TestEntry tooLongDir13;
	TestEntry tooLongDir14;
	TestEntry tooLongDir15;
	TestEntry tooLongDir16;

	void _ConstructorDirPathTest(TestEntry& _testEntry, status_t error, bool traverse = false)
	{
		TestEntry* testEntry = &_testEntry;
		// absolute path
		{
			if (!testEntry->isBad() && testEntry->path.length() < B_PATH_NAME_LENGTH) {
				BDirectory dir("/boot/home/Desktop");
				CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
				BEntry entry(&dir, testEntry->cpath, traverse);
				status_t result = entry.InitCheck();
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				if (error == B_OK)
					examine_entry(entry, testEntry, traverse);
			}
		}
		// relative path (one level)
		{
			if (!testEntry->isBad() && testEntry->super->path.length() < B_PATH_NAME_LENGTH) {
				BDirectory dir(testEntry->super->cpath);
				CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
				BEntry entry(&dir, testEntry->cname, traverse);
				status_t result = entry.InitCheck();
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				if (error == B_OK)
					examine_entry(entry, testEntry, traverse);
			}
		}
		// relative path (two levels)
		{
			if (!testEntry->super->isBad() && !testEntry->super->super->isBad()) {
				std::string entryName = testEntry->super->name + "/" + testEntry->name;
				BDirectory dir(testEntry->super->super->cpath);
				CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
				BEntry entry(&dir, entryName.c_str(), traverse);
				status_t result = entry.InitCheck();
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				if (error == B_OK)
					examine_entry(entry, testEntry, traverse);
			}
		}
	}

	void _ConstructorRefTest(TestEntry& _testEntry, status_t error, bool traverse = false)
	{
		TestEntry* testEntry = &_testEntry;
		// absolute path
		{
			BEntry entry(&testEntry->get_ref(), traverse);
			status_t result = entry.InitCheck();
			CPPUNIT_ASSERT(fuzzy_equals(result, error));
			if (error == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}

	void _ConstructorPathTest(TestEntry& _testEntry, status_t error, bool traverse = false)
	{
		TestEntry* testEntry = &_testEntry;
		// absolute path
		{
			BEntry entry(testEntry->cpath, traverse);
			status_t result = entry.InitCheck();
			CPPUNIT_ASSERT(fuzzy_equals(result, error));
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
		// relative path
		{
			if (chdir(testEntry->super->cpath) == 0) {
				BEntry entry(testEntry->cname, traverse);
				status_t result = entry.InitCheck();
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				if (error == B_OK)
					examine_entry(entry, testEntry, traverse);
				RestoreCWD();
			}
		}
	}

	void _SetToDirPathTest(TestEntry& _testEntry, status_t error, bool traverse = false)
	{
		TestEntry* testEntry = &_testEntry;
		BEntry entry;
		// absolute path
		{
			if (!testEntry->isBad() && testEntry->path.length() < B_PATH_NAME_LENGTH) {
				BDirectory dir("/boot/home/Desktop");
				CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
				status_t result = entry.SetTo(&dir, testEntry->cpath, traverse);
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				CPPUNIT_ASSERT(fuzzy_equals(entry.InitCheck(), error));
				if (result == B_OK)
					examine_entry(entry, testEntry, traverse);
			}
		}
		// relative path (one level)
		{
			if (!testEntry->isBad() && testEntry->super->path.length() < B_PATH_NAME_LENGTH) {
				BDirectory dir(testEntry->super->cpath);
				CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
				status_t result = entry.SetTo(&dir, testEntry->cname, traverse);
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				CPPUNIT_ASSERT(fuzzy_equals(entry.InitCheck(), error));
				if (result == B_OK)
					examine_entry(entry, testEntry, traverse);
			}
		}
		// relative path (two levels)
		{
			if (!testEntry->super->isBad() && !testEntry->super->super->isBad()) {
				std::string entryName = testEntry->super->name + "/" + testEntry->name;
				BDirectory dir(testEntry->super->super->cpath);
				CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
				status_t result = entry.SetTo(&dir, entryName.c_str(), traverse);
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				CPPUNIT_ASSERT(fuzzy_equals(entry.InitCheck(), error));
				if (result == B_OK)
					examine_entry(entry, testEntry, traverse);
			}
		}
	}

	void _SetToRefTest(TestEntry& _testEntry, status_t error, bool traverse = false)
	{
		TestEntry* testEntry = &_testEntry;
		BEntry entry;
		// absolute path
		{
			status_t result = entry.SetTo(&testEntry->get_ref(), traverse);
			CPPUNIT_ASSERT(fuzzy_equals(result, error));
			CPPUNIT_ASSERT(fuzzy_equals(entry.InitCheck(), error));
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}

	void _SetToPathTest(TestEntry& _testEntry, status_t error, bool traverse = false)
	{
		TestEntry* testEntry = &_testEntry;
		BEntry entry;
		// absolute path
		{
			status_t result = entry.SetTo(testEntry->cpath, traverse);
			CPPUNIT_ASSERT(fuzzy_equals(result, error));
			CPPUNIT_ASSERT(fuzzy_equals(entry.InitCheck(), error));
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
		// relative path
		{
			if (chdir(testEntry->super->cpath) == 0) {
				status_t result = entry.SetTo(testEntry->cname, traverse);
				CPPUNIT_ASSERT(fuzzy_equals(result, error));
				CPPUNIT_ASSERT(fuzzy_equals(entry.InitCheck(), error));
				if (result == B_OK)
					examine_entry(entry, testEntry, traverse);
				RestoreCWD();
			}
		}
	}
	void _MoveToTestLink(TestEntry* testEntry, TestEntry* testDir, TestEntry* newTestEntry,
		bool existing, bool clobber, status_t error)
	{
		_MoveToTestEntry(testEntry, testDir, newTestEntry, existing, clobber, error,
			B_SYMLINK_NODE);
	}

	void _MoveToTestDir(TestEntry* testEntry, TestEntry* testDir, TestEntry* newTestEntry,
		bool existing, bool clobber, status_t error)
	{
		_MoveToTestEntry(testEntry, testDir, newTestEntry, existing, clobber, error,
			B_DIRECTORY_NODE);
	}

	void _MoveToTestFile(TestEntry* testEntry, TestEntry* testDir, TestEntry* newTestEntry,
		bool existing, bool clobber, status_t error)
	{
		_MoveToTestEntry(testEntry, testDir, newTestEntry, existing, clobber, error, B_FILE_NODE);
	}

	void _MoveToTestEntry(TestEntry* testEntry, TestEntry* testDir, TestEntry* newTestEntry,
		bool existing, bool clobber, status_t error, uint32 kind)
	{
		if (newTestEntry) {
			// Here is the right place to play a little bit with the dir and path
			// arguments. At this time we only pass the leaf name and the
			// absolute path name.
			_MoveToTestEntry(testEntry, testDir, newTestEntry->name, existing, clobber, error,
				kind);
			_MoveToTestEntry(testEntry, &subDir1, newTestEntry->path, existing, clobber, error,
				kind);
		} else {
			_MoveToTestEntry(testEntry, testDir, "", existing, clobber, error, kind);
		}
	}

	void _MoveToTestEntry(TestEntry* testEntry, TestEntry* testDir, std::string newName, bool existing,
		bool clobber, status_t error, uint32 kind)
	{
		BEntry entry;
		BDirectory dir;
		// get all the names
		std::string pathname = testEntry->path;
		std::string dirname = testDir->path;
		std::string newPathname = dirname + "/";
		if (newName.length() == 0) {
			newPathname += testEntry->name;
		} else {
			// check, if the new path is absolute
			if (newName.find("/") == 0)
				newPathname = newName;
			else
				newPathname += newName;
		}
		// printf("path: `%s', dir: `%s', new name: `%s'\n", pathname.c_str(),
		// dirname.c_str(), newPathname.c_str());
		//  create the entries
		switch (kind) {
			case B_FILE_NODE:
				CreateFile(pathname.c_str());
				break;
			case B_DIRECTORY_NODE:
				CreateDir(pathname.c_str());
				break;
			case B_SYMLINK_NODE:
				CreateLink(pathname.c_str(), file1.cpath);
				break;
		}
		if (existing)
			CreateFile(newPathname.c_str());
		// move the file
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(pathname.c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dirname.c_str()));
		if (newName.length() == 0) {
			status_t result = entry.MoveTo(&dir, NULL, clobber);
			if (result != error) {
				printf("`%s'.MoveTo(`%s', NULL, %d): ", pathname.c_str(), dirname.c_str(), clobber);
				printf("error: %lx (%lx)\n", (unsigned long)result, (unsigned long)error);
			}
			CPPUNIT_ASSERT(result == error);
		} else {
			status_t result = entry.MoveTo(&dir, newName.c_str(), clobber);
			if (result != error) {
				printf("`%s'.MoveTo(`%s', `%s', %d): ", pathname.c_str(), newName.c_str(),
					dirname.c_str(), clobber);
				printf("error: %lx (%lx)\n", (unsigned long)result, (unsigned long)error);
			}
			CPPUNIT_ASSERT(result == error);
		}
		// check and cleanup
		if (error == B_OK) {
			switch (kind) {
				case B_FILE_NODE:
					CPPUNIT_ASSERT(!PingFile(pathname.c_str()));
					CPPUNIT_ASSERT(PingFile(newPathname.c_str()));
					break;
				case B_DIRECTORY_NODE:
					CPPUNIT_ASSERT(!PingDir(pathname.c_str()));
					CPPUNIT_ASSERT(PingDir(newPathname.c_str()));
					break;
				case B_SYMLINK_NODE:
					CPPUNIT_ASSERT(!PingLink(pathname.c_str()));
					CPPUNIT_ASSERT(PingLink(newPathname.c_str(), file1.cpath));
					break;
			}
			RemoveFile(newPathname.c_str());
		} else {
			switch (kind) {
				case B_FILE_NODE:
					CPPUNIT_ASSERT(PingFile(pathname.c_str()));
					break;
				case B_DIRECTORY_NODE:
					CPPUNIT_ASSERT(PingDir(pathname.c_str()));
					break;
				case B_SYMLINK_NODE:
					CPPUNIT_ASSERT(PingLink(pathname.c_str(), file1.cpath));
					break;
			}
			if (existing) {
				CPPUNIT_ASSERT(PingFile(newPathname.c_str()));
				RemoveFile(newPathname.c_str());
			}
			RemoveFile(pathname.c_str());
		}
	}

	void _RenameTestLink(TestEntry* testEntry, TestEntry* newTestEntry, bool existing, bool clobber,
		status_t error)
	{
		_RenameTestEntry(testEntry, newTestEntry, existing, clobber, error, B_SYMLINK_NODE);
	}

	void _RenameTestDir(TestEntry* testEntry, TestEntry* newTestEntry, bool existing, bool clobber,
		status_t error)
	{
		_RenameTestEntry(testEntry, newTestEntry, existing, clobber, error, B_DIRECTORY_NODE);
	}

	void _RenameTestFile(TestEntry* testEntry, TestEntry* newTestEntry, bool existing, bool clobber,
		status_t error)
	{
		_RenameTestEntry(testEntry, newTestEntry, existing, clobber, error, B_FILE_NODE);
	}

	void _RenameTestEntry(TestEntry* testEntry, TestEntry* newTestEntry, bool existing,
		bool clobber, status_t error, uint32 kind)
	{
		// relative path
		std::string relPath = get_shortest_relative_path(testEntry->super, newTestEntry);
		if (relPath.length() > 0)
			_RenameTestEntry(testEntry, newTestEntry, relPath, existing, clobber, error, kind);
		// absolute path
		_RenameTestEntry(testEntry, newTestEntry, newTestEntry->path, existing, clobber, error,
			kind);
	}

	void _RenameTestEntry(TestEntry* testEntry, TestEntry* newTestEntry, std::string newName,
		bool existing, bool clobber, status_t error, uint32 kind)
	{
		BEntry entry;
		BDirectory dir;
		// get all the names
		std::string pathname = testEntry->path;
		std::string dirname = testEntry->super->path;
		std::string newPathname = newTestEntry->path;
		// printf("path: `%s', dir: `%s', new name: `%s'\n", pathname.c_str(),
		// dirname.c_str(), newPathname.c_str());
		//  create the entries
		switch (kind) {
			case B_FILE_NODE:
				CreateFile(pathname.c_str());
				break;
			case B_DIRECTORY_NODE:
				CreateDir(pathname.c_str());
				break;
			case B_SYMLINK_NODE:
				CreateLink(pathname.c_str(), file1.cpath);
				break;
		}
		if (existing)
			CreateFile(newPathname.c_str());
		// rename the file
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(pathname.c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dirname.c_str()));
		status_t result = entry.Rename(newName.c_str(), clobber);
		if (result != error) {
			printf("`%s'.Rename(`%s', %d): ", pathname.c_str(), newName.c_str(), clobber);
			printf("error: %lx (%lx)\n", (unsigned long)result, (unsigned long)error);
		}
		CPPUNIT_ASSERT(result == error);
		// check and cleanup
		if (error == B_OK) {
			switch (kind) {
				case B_FILE_NODE:
					CPPUNIT_ASSERT(!PingFile(pathname.c_str()));
					CPPUNIT_ASSERT(PingFile(newPathname.c_str()));
					break;
				case B_DIRECTORY_NODE:
					CPPUNIT_ASSERT(!PingDir(pathname.c_str()));
					CPPUNIT_ASSERT(PingDir(newPathname.c_str()));
					break;
				case B_SYMLINK_NODE:
					CPPUNIT_ASSERT(!PingLink(pathname.c_str()));
					CPPUNIT_ASSERT(PingLink(newPathname.c_str(), file1.cpath));
					break;
			}
			RemoveFile(newPathname.c_str());
		} else {
			switch (kind) {
				case B_FILE_NODE:
					CPPUNIT_ASSERT(PingFile(pathname.c_str()));
					break;
				case B_DIRECTORY_NODE:
					CPPUNIT_ASSERT(PingDir(pathname.c_str()));
					break;
				case B_SYMLINK_NODE:
					CPPUNIT_ASSERT(PingLink(pathname.c_str(), file1.cpath));
					break;
			}
			if (existing) {
				CPPUNIT_ASSERT(PingFile(newPathname.c_str()));
				RemoveFile(newPathname.c_str());
			}
			RemoveFile(pathname.c_str());
		}
	}

protected:
	void CreateUninitializedStatables(TestStatables& testEntries)
	{
		testEntries.add(new BEntry, "");
	}

	void CreateRWStatables(TestStatables& testStatables)
	{
		TestEntry* testEntries[] = {&dir1, &dir2, &file1, &subDir1, &absDirLink1, &absDirLink2,
			&absDirLink3, &absDirLink4, &relDirLink1, &relDirLink2, &relDirLink3, &relDirLink4,
			&absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4, &relFileLink1,
			&relFileLink2, &relFileLink3, &relFileLink4, &absBadLink1, &absBadLink2, &absBadLink3,
			&absBadLink4, &relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4, &absVeryBadLink1,
			&absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4, &relVeryBadLink1,
			&relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4};
		int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
		for (int32 i = 0; i < testEntryCount; i++) {
			TestEntry* testEntry = testEntries[i];
			const char* filename = testEntry->cpath;
			testStatables.add(new BEntry(filename), filename);
		}
	}

	void CreateROStatables(TestStatables& testEntries) { CreateRWStatables(testEntries); }

	bool PingLink(const char* path, const char* target = NULL, BEntry* entry = NULL)
	{
		bool result = false;
		// check existence and type
		struct stat st;
		if (lstat(path, &st) == 0)
			result = (S_ISLNK(st.st_mode));
		// check target
		if (result && target) {
			char linkTarget[B_PATH_NAME_LENGTH + 1];
			ssize_t size = readlink(path, linkTarget, B_PATH_NAME_LENGTH);
			result = (size >= 0);
			if (result) {
				linkTarget[size] = 0;
				result = (std::string (linkTarget) == target);
			}
		}
		// check entry
		if (result && entry) {
			BPath entryPath;
			result = (entry->GetPath(&entryPath) == B_OK && entryPath == path);
		}
		return result;
	}

	bool PingDir(const char* path, BEntry* entry = NULL)
	{
		bool result = false;
		// check existence and type
		struct stat st;
		if (lstat(path, &st) == 0)
			result = (S_ISDIR(st.st_mode));
		// check entry
		if (result && entry) {
			BPath entryPath;
			result = (entry->GetPath(&entryPath) == B_OK && entryPath == path);
		}
		return result;
	}

	bool PingFile(const char* path, BEntry* entry = NULL)
	{
		bool result = false;
		// check existence and type
		struct stat st;
		if (lstat(path, &st) == 0)
			result = (S_ISREG(st.st_mode));
		// check entry
		if (result && entry) {
			BPath entryPath;
			result = (entry->GetPath(&entryPath) == B_OK && entryPath == path);
		}
		return result;
	}

	void RemoveFile(const char* file)
	{
		if (file && isHarmlessPathname(file))
			unlink(file);
	}

	void CreateDir(const char* dir)
	{
		if (dir && isHarmlessPathname(dir))
			mkdir(dir, S_IRWXU);
	}

	void CreateFile(const char* file)
	{
		if (file && isHarmlessPathname(file))
			creat(file, S_IRWXU);
	}

	void CreateLink(const char* link, const char* target)
	{
		if (link && target && isHarmlessPathname(link))
			symlink(target, link);
	}

public:
	EntryTest()
	{
		// root dir for testing
		testDir.initDir(badTestEntry, "testDir");
		testDir.initPath((std::string ("/tmp/") + testDir.name).c_str());
		allTestEntries.pop_back();
		// other entries
		dir1.initDir(testDir, "dir1");
		dir2.initDir(testDir, "dir2");
		file1.initFile(dir1, "file1");
		file2.init(dir1, "file2", ABSTRACT_ENTRY);
		file3.init(dir2, "file3", ABSTRACT_ENTRY);
		file4.init(dir1, "file4", ABSTRACT_ENTRY);
		subDir1.initDir(dir1, "subDir1");
		subDir2.init(dir1, "subDir2", ABSTRACT_ENTRY);
		subDir3.init(dir2, "subDir3", ABSTRACT_ENTRY);
		abstractEntry1.init(dir1, "abstractEntry1", ABSTRACT_ENTRY);
		badEntry1.init(abstractEntry1, "badEntry1", BAD_ENTRY);
		absDirLink1.initALink(dir1, "absDirLink1", subDir1);
		absDirLink2.initALink(dir1, "absDirLink2", absDirLink1);
		absDirLink3.initALink(dir2, "absDirLink3", absDirLink2);
		absDirLink4.initALink(testDir, "absDirLink4", absDirLink3);
		relDirLink1.initRLink(dir1, "relDirLink1", subDir1);
		relDirLink2.initRLink(dir1, "relDirLink2", relDirLink1);
		relDirLink3.initRLink(dir2, "relDirLink3", relDirLink2);
		relDirLink4.initRLink(testDir, "relDirLink4", relDirLink3);
		absFileLink1.initALink(dir1, "absFileLink1", file1);
		absFileLink2.initALink(dir1, "absFileLink2", absFileLink1);
		absFileLink3.initALink(dir2, "absFileLink3", absFileLink2);
		absFileLink4.initALink(testDir, "absFileLink4", absFileLink3);
		relFileLink1.initRLink(dir1, "relFileLink1", file1);
		relFileLink2.initRLink(dir1, "relFileLink2", relFileLink1);
		relFileLink3.initRLink(dir2, "relFileLink3", relFileLink2);
		relFileLink4.initRLink(testDir, "relFileLink4", relFileLink3);
		absCyclicLink1.initALink(dir1, "absCyclicLink1", absCyclicLink2);
		absCyclicLink2.initALink(dir1, "absCyclicLink2", absCyclicLink1);
		relCyclicLink1.initRLink(dir1, "relCyclicLink1", relCyclicLink2);
		relCyclicLink2.initRLink(dir1, "relCyclicLink2", relCyclicLink1);
		absBadLink1.initALink(dir1, "absBadLink1", abstractEntry1);
		absBadLink2.initALink(dir1, "absBadLink2", absBadLink1);
		absBadLink3.initALink(dir2, "absBadLink3", absBadLink2);
		absBadLink4.initALink(testDir, "absBadLink4", absBadLink3);
		relBadLink1.initRLink(dir1, "relBadLink1", abstractEntry1);
		relBadLink2.initRLink(dir1, "relBadLink2", relBadLink1);
		relBadLink3.initRLink(dir2, "relBadLink3", relBadLink2);
		relBadLink4.initRLink(testDir, "relBadLink4", relBadLink3);
		absVeryBadLink1.initALink(dir1, "absVeryBadLink1", badEntry1);
		absVeryBadLink2.initALink(dir1, "absVeryBadLink2", absVeryBadLink1);
		absVeryBadLink3.initALink(dir2, "absVeryBadLink3", absVeryBadLink2);
		absVeryBadLink4.initALink(testDir, "absVeryBadLink4", absVeryBadLink3);
		relVeryBadLink1.initRLink(dir1, "relVeryBadLink1", badEntry1);
		relVeryBadLink2.initRLink(dir1, "relVeryBadLink2", relVeryBadLink1);
		relVeryBadLink3.initRLink(dir2, "relVeryBadLink3", relVeryBadLink2);
		relVeryBadLink4.initRLink(testDir, "relVeryBadLink4", relVeryBadLink3);
		tooLongEntry1.init(testDir, tooLongEntryname, ABSTRACT_ENTRY);
		tooLongDir1.initDir(testDir, tooLongDirname);
		tooLongDir2.initDir(tooLongDir1, tooLongDirname);
		tooLongDir3.initDir(tooLongDir2, tooLongDirname);
		tooLongDir4.initDir(tooLongDir3, tooLongDirname);
		tooLongDir5.initDir(tooLongDir4, tooLongDirname);
		tooLongDir6.initDir(tooLongDir5, tooLongDirname);
		tooLongDir7.initDir(tooLongDir6, tooLongDirname);
		tooLongDir8.initDir(tooLongDir7, tooLongDirname);
		tooLongDir9.initDir(tooLongDir8, tooLongDirname);
		tooLongDir10.initDir(tooLongDir9, tooLongDirname);
		tooLongDir11.initDir(tooLongDir10, tooLongDirname);
		tooLongDir12.initDir(tooLongDir11, tooLongDirname);
		tooLongDir13.initDir(tooLongDir12, tooLongDirname);
		tooLongDir14.initDir(tooLongDir13, tooLongDirname);
		tooLongDir15.initDir(tooLongDir14, tooLongDirname);
		tooLongDir16.initDir(tooLongDir15, tooLongDirname);

		// init paths
		for (std::vector<TestEntry*>::iterator it = allTestEntries.begin(); it != allTestEntries.end(); it++)
			(*it)->initPath();
		// complete initialization
		testDir.completeInit();
		for (std::vector<TestEntry*>::iterator it = allTestEntries.begin(); it != allTestEntries.end(); it++)
			(*it)->completeInit();
		// create the set up command line
		setUpCommandLine = std::string ("mkdir ") + testDir.path;
		for (std::vector<TestEntry*>::iterator it = allTestEntries.begin(); it != allTestEntries.end(); it++) {
			TestEntry* entry = *it;
			std::string command;
			switch (entry->kind) {
				case ABSTRACT_ENTRY:
					break;
				case DIR_ENTRY:
				{
					if (entry->path.length() < B_PATH_NAME_LENGTH) {
						command = std::string ("mkdir ") + entry->path;
					} else {
						command = std::string ("( ") + "cd " + entry->super->super->path + " ; " + " mkdir "
							+ entry->super->name + "/" + entry->name + " )";
					}
					break;
				}
				case FILE_ENTRY:
				{
					command = std::string ("touch ") + entry->path;
					break;
				}
				case LINK_ENTRY:
				{
					command = std::string ("ln -s ") + entry->link + " " + entry->path;
					break;
				}
				default:
					break;
			}
			if (command.length() > 0) {
				if (setUpCommandLine.length() == 0)
					setUpCommandLine = command;
				else
					setUpCommandLine += std::string (" ; ") + command;
			}
		}
		// create the tear down command line
		tearDownCommandLine = std::string ("rm -rf ") + testDir.path;
	}

	void setUp()
	{
		StatableTest::setUp();
		execCommand(setUpCommandLine);
	}

	void tearDown()
	{
		StatableTest::tearDown();
		execCommand(tearDownCommandLine);
	}

	void DefaultConstructor_ReturnsNoInit()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
	}

	void Existing_PathConstructor_ReturnsExpected()
	{
		_ConstructorPathTest(dir1, B_OK);
		_ConstructorPathTest(dir2, B_OK);
		_ConstructorPathTest(file1, B_OK);
		_ConstructorPathTest(subDir1, B_OK);
		_ConstructorPathTest(abstractEntry1, B_OK);

		_ConstructorPathTest(dir1, B_OK, true);
		_ConstructorPathTest(dir2, B_OK, true);
		_ConstructorPathTest(file1, B_OK, true);
		_ConstructorPathTest(subDir1, B_OK, true);
		_ConstructorPathTest(abstractEntry1, B_OK, true);
	}

	void Link_PathConstructor_ReturnsExpected()
	{
		// don't traverse
		_ConstructorPathTest(absDirLink1, B_OK);
		_ConstructorPathTest(absDirLink2, B_OK);
		_ConstructorPathTest(absDirLink3, B_OK);
		_ConstructorPathTest(absDirLink4, B_OK);
		_ConstructorPathTest(relDirLink1, B_OK);
		_ConstructorPathTest(relDirLink2, B_OK);
		_ConstructorPathTest(relDirLink3, B_OK);
		_ConstructorPathTest(relDirLink4, B_OK);
		_ConstructorPathTest(absFileLink1, B_OK);
		_ConstructorPathTest(absFileLink2, B_OK);
		_ConstructorPathTest(absFileLink3, B_OK);
		_ConstructorPathTest(absFileLink4, B_OK);
		_ConstructorPathTest(relFileLink1, B_OK);
		_ConstructorPathTest(relFileLink2, B_OK);
		_ConstructorPathTest(relFileLink3, B_OK);
		_ConstructorPathTest(relFileLink4, B_OK);

		// traverse
		_ConstructorPathTest(absDirLink1, B_OK, true);
		_ConstructorPathTest(absDirLink2, B_OK, true);
		_ConstructorPathTest(absDirLink3, B_OK, true);
		_ConstructorPathTest(absDirLink4, B_OK, true);
		_ConstructorPathTest(relDirLink1, B_OK, true);
		_ConstructorPathTest(relDirLink2, B_OK, true);
		_ConstructorPathTest(relDirLink3, B_OK, true);
		_ConstructorPathTest(relDirLink4, B_OK, true);
		_ConstructorPathTest(absFileLink1, B_OK, true);
		_ConstructorPathTest(absFileLink2, B_OK, true);
		_ConstructorPathTest(absFileLink3, B_OK, true);
		_ConstructorPathTest(absFileLink4, B_OK, true);
		_ConstructorPathTest(relFileLink1, B_OK, true);
		_ConstructorPathTest(relFileLink2, B_OK, true);
		_ConstructorPathTest(relFileLink3, B_OK, true);
		_ConstructorPathTest(relFileLink4, B_OK, true);
	}

	void BadLink_PathConstructor_ReturnsExpected()
	{
		_ConstructorPathTest(badEntry1, B_ENTRY_NOT_FOUND);
		_ConstructorPathTest(absBadLink1, B_OK);
		_ConstructorPathTest(absBadLink2, B_OK);
		_ConstructorPathTest(absBadLink3, B_OK);
		_ConstructorPathTest(absBadLink4, B_OK);
		_ConstructorPathTest(relBadLink1, B_OK);
		_ConstructorPathTest(relBadLink2, B_OK);
		_ConstructorPathTest(relBadLink3, B_OK);
		_ConstructorPathTest(relBadLink4, B_OK);
		_ConstructorPathTest(absVeryBadLink1, B_OK);
		_ConstructorPathTest(absVeryBadLink2, B_OK);
		_ConstructorPathTest(absVeryBadLink3, B_OK);
		_ConstructorPathTest(absVeryBadLink4, B_OK);
		_ConstructorPathTest(relVeryBadLink1, B_OK);
		_ConstructorPathTest(relVeryBadLink2, B_OK);
		_ConstructorPathTest(relVeryBadLink3, B_OK);
		_ConstructorPathTest(relVeryBadLink4, B_OK);

		// traverse
		_ConstructorPathTest(badEntry1, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorPathTest(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	}

	void CyclicLink_PathConstructor_ReturnsExpected()
	{
		_ConstructorPathTest(absCyclicLink1, B_OK);
		_ConstructorPathTest(relCyclicLink1, B_OK);
		_ConstructorPathTest(absCyclicLink1, B_LINK_LIMIT, true);
		_ConstructorPathTest(relCyclicLink1, B_LINK_LIMIT, true);
	}

	void TooLong_PathConstructor_ReturnsExpected()
	{
		// R5: returns E2BIG instead of B_NAME_TOO_LONG
		_ConstructorPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
		// R5: returns B_ERROR instead of B_NAME_TOO_LONG
		_ConstructorPathTest(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG));

		_ConstructorPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
		_ConstructorPathTest(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG), true);
	}

	void Root_PathConstructor_ReturnsOk()
	{
		BEntry entry("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void Boot_PathConstructor_ReturnsOk()
	{
		BEntry entry("/boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void Null_PathConstructor_ReturnsBadValue()
	{
		BEntry entry((const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
	}

	void Existing_RefConstructor_ReturnsExpected()
	{
		_ConstructorRefTest(dir1, B_OK);
		_ConstructorRefTest(dir2, B_OK);
		_ConstructorRefTest(file1, B_OK);
		_ConstructorRefTest(subDir1, B_OK);
		_ConstructorRefTest(abstractEntry1, B_OK);

		_ConstructorRefTest(dir1, B_OK, true);
		_ConstructorRefTest(dir2, B_OK, true);
		_ConstructorRefTest(file1, B_OK, true);
		_ConstructorRefTest(subDir1, B_OK, true);
		_ConstructorRefTest(abstractEntry1, B_OK, true);
	}

	void Link_RefConstructor_ReturnsExpected()
	{
		// don't traverse
		_ConstructorRefTest(absDirLink1, B_OK);
		_ConstructorRefTest(absDirLink2, B_OK);
		_ConstructorRefTest(absDirLink3, B_OK);
		_ConstructorRefTest(absDirLink4, B_OK);
		_ConstructorRefTest(relDirLink1, B_OK);
		_ConstructorRefTest(relDirLink2, B_OK);
		_ConstructorRefTest(relDirLink3, B_OK);
		_ConstructorRefTest(relDirLink4, B_OK);
		_ConstructorRefTest(absFileLink1, B_OK);
		_ConstructorRefTest(absFileLink2, B_OK);
		_ConstructorRefTest(absFileLink3, B_OK);
		_ConstructorRefTest(absFileLink4, B_OK);
		_ConstructorRefTest(relFileLink1, B_OK);
		_ConstructorRefTest(relFileLink2, B_OK);
		_ConstructorRefTest(relFileLink3, B_OK);
		_ConstructorRefTest(relFileLink4, B_OK);

		// traverse
		_ConstructorRefTest(absDirLink1, B_OK, true);
		_ConstructorRefTest(absDirLink2, B_OK, true);
		_ConstructorRefTest(absDirLink3, B_OK, true);
		_ConstructorRefTest(absDirLink4, B_OK, true);
		_ConstructorRefTest(relDirLink1, B_OK, true);
		_ConstructorRefTest(relDirLink2, B_OK, true);
		_ConstructorRefTest(relDirLink3, B_OK, true);
		_ConstructorRefTest(relDirLink4, B_OK, true);
		_ConstructorRefTest(absFileLink1, B_OK, true);
		_ConstructorRefTest(absFileLink2, B_OK, true);
		_ConstructorRefTest(absFileLink3, B_OK, true);
		_ConstructorRefTest(absFileLink4, B_OK, true);
		_ConstructorRefTest(relFileLink1, B_OK, true);
		_ConstructorRefTest(relFileLink2, B_OK, true);
		_ConstructorRefTest(relFileLink3, B_OK, true);
		_ConstructorRefTest(relFileLink4, B_OK, true);
	}

	void BadLink_RefConstructor_ReturnsExpected()
	{
		_ConstructorRefTest(absBadLink1, B_OK);
		_ConstructorRefTest(absBadLink2, B_OK);
		_ConstructorRefTest(absBadLink3, B_OK);
		_ConstructorRefTest(absBadLink4, B_OK);
		_ConstructorRefTest(relBadLink1, B_OK);
		_ConstructorRefTest(relBadLink2, B_OK);
		_ConstructorRefTest(relBadLink3, B_OK);
		_ConstructorRefTest(relBadLink4, B_OK);
		_ConstructorRefTest(absVeryBadLink1, B_OK);
		_ConstructorRefTest(absVeryBadLink2, B_OK);
		_ConstructorRefTest(absVeryBadLink3, B_OK);
		_ConstructorRefTest(absVeryBadLink4, B_OK);
		_ConstructorRefTest(relVeryBadLink1, B_OK);
		_ConstructorRefTest(relVeryBadLink2, B_OK);
		_ConstructorRefTest(relVeryBadLink3, B_OK);
		_ConstructorRefTest(relVeryBadLink4, B_OK);

		// traverse
		_ConstructorRefTest(absBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorRefTest(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	}

	void CyclicLink_RefConstructor_ReturnsExpected()
	{
		_ConstructorRefTest(absCyclicLink1, B_OK);
		_ConstructorRefTest(relCyclicLink1, B_OK);
		_ConstructorRefTest(absCyclicLink1, B_LINK_LIMIT, true);
		_ConstructorRefTest(relCyclicLink1, B_LINK_LIMIT, true);
	}

	void Null_RefConstructor_ReturnsExpected()
	{
		BEntry entry((const entry_ref*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
	}

	void Existing_DirPathConstructor_ReturnsExpected()
	{
		_ConstructorDirPathTest(dir1, B_OK);
		_ConstructorDirPathTest(dir2, B_OK);
		_ConstructorDirPathTest(file1, B_OK);
		_ConstructorDirPathTest(subDir1, B_OK);
		_ConstructorDirPathTest(abstractEntry1, B_OK);

		_ConstructorDirPathTest(dir1, B_OK, true);
		_ConstructorDirPathTest(dir2, B_OK, true);
		_ConstructorDirPathTest(file1, B_OK, true);
		_ConstructorDirPathTest(subDir1, B_OK, true);
		_ConstructorDirPathTest(abstractEntry1, B_OK, true);
	}

	void Link_DirPathConstructor_ReturnsExpected()
	{
		// don't traverse
		_ConstructorDirPathTest(absDirLink1, B_OK);
		_ConstructorDirPathTest(absDirLink2, B_OK);
		_ConstructorDirPathTest(absDirLink3, B_OK);
		_ConstructorDirPathTest(absDirLink4, B_OK);
		_ConstructorDirPathTest(relDirLink1, B_OK);
		_ConstructorDirPathTest(relDirLink2, B_OK);
		_ConstructorDirPathTest(relDirLink3, B_OK);
		_ConstructorDirPathTest(relDirLink4, B_OK);
		_ConstructorDirPathTest(absFileLink1, B_OK);
		_ConstructorDirPathTest(absFileLink2, B_OK);
		_ConstructorDirPathTest(absFileLink3, B_OK);
		_ConstructorDirPathTest(absFileLink4, B_OK);
		_ConstructorDirPathTest(relFileLink1, B_OK);
		_ConstructorDirPathTest(relFileLink2, B_OK);
		_ConstructorDirPathTest(relFileLink3, B_OK);
		_ConstructorDirPathTest(relFileLink4, B_OK);

		// traverse
		_ConstructorDirPathTest(absDirLink1, B_OK, true);
		_ConstructorDirPathTest(absDirLink2, B_OK, true);
		_ConstructorDirPathTest(absDirLink3, B_OK, true);
		_ConstructorDirPathTest(absDirLink4, B_OK, true);
		_ConstructorDirPathTest(relDirLink1, B_OK, true);
		_ConstructorDirPathTest(relDirLink2, B_OK, true);
		_ConstructorDirPathTest(relDirLink3, B_OK, true);
		_ConstructorDirPathTest(relDirLink4, B_OK, true);
		_ConstructorDirPathTest(absFileLink1, B_OK, true);
		_ConstructorDirPathTest(absFileLink2, B_OK, true);
		_ConstructorDirPathTest(absFileLink3, B_OK, true);
		_ConstructorDirPathTest(absFileLink4, B_OK, true);
		_ConstructorDirPathTest(relFileLink1, B_OK, true);
		_ConstructorDirPathTest(relFileLink2, B_OK, true);
		_ConstructorDirPathTest(relFileLink3, B_OK, true);
		_ConstructorDirPathTest(relFileLink4, B_OK, true);
	}

	void BadLink_DirPathConstructor_ReturnsExpected()
	{
		_ConstructorDirPathTest(badEntry1, B_ENTRY_NOT_FOUND);
		_ConstructorDirPathTest(absBadLink1, B_OK);
		_ConstructorDirPathTest(absBadLink2, B_OK);
		_ConstructorDirPathTest(absBadLink3, B_OK);
		_ConstructorDirPathTest(absBadLink4, B_OK);
		_ConstructorDirPathTest(relBadLink1, B_OK);
		_ConstructorDirPathTest(relBadLink2, B_OK);
		_ConstructorDirPathTest(relBadLink3, B_OK);
		_ConstructorDirPathTest(relBadLink4, B_OK);
		_ConstructorDirPathTest(absVeryBadLink1, B_OK);
		_ConstructorDirPathTest(absVeryBadLink2, B_OK);
		_ConstructorDirPathTest(absVeryBadLink3, B_OK);
		_ConstructorDirPathTest(absVeryBadLink4, B_OK);
		_ConstructorDirPathTest(relVeryBadLink1, B_OK);
		_ConstructorDirPathTest(relVeryBadLink2, B_OK);
		_ConstructorDirPathTest(relVeryBadLink3, B_OK);
		_ConstructorDirPathTest(relVeryBadLink4, B_OK);

		// traverse
		_ConstructorDirPathTest(badEntry1, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_ConstructorDirPathTest(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	}

	void CyclicLink_DirPathConstructor_ReturnsExpected()
	{
		_ConstructorDirPathTest(absCyclicLink1, B_OK);
		_ConstructorDirPathTest(relCyclicLink1, B_OK);
		_ConstructorDirPathTest(absCyclicLink1, B_LINK_LIMIT, true);
		_ConstructorDirPathTest(relCyclicLink1, B_LINK_LIMIT, true);
	}

	void TooLong_DirPathConstructor_ReturnsExpected()
	{
		// R5: returns E2BIG instead of B_NAME_TOO_LONG
		_ConstructorDirPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
// Haiku: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
		_ConstructorDirPathTest(tooLongDir16, B_OK);
#endif

		// R5: returns E2BIG instead of B_NAME_TOO_LONG
		_ConstructorDirPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
// Haiku: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
		_ConstructorDirPathTest(tooLongDir16, B_OK, true);
#endif
	}

	void RootDotLeaf_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir("/");
		BEntry entry(&dir, ".");
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void RootBootApplicationLeaf_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir("/");
		BEntry entry(&dir, "boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void NullLeaf_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir("/");
		BEntry entry(&dir, (const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void NullDirAndValidLeaf_DirPathConstructor_ReturnsBadValue()
	{
#if !TEST_R5
		chdir("/");
		BEntry entry((const BDirectory*)NULL, "tmp");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
		RestoreCWD();
#endif
	}

	void NullDirAndNullLeaf_DirPathConstructor_ReturnsBadValue()
	{
#if !TEST_R5
		BEntry entry((const BDirectory*)NULL, (const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
#endif
	}

	void NullDirAndAbsolutePath_DirPathConstructor_ReturnsBadValue()
	{
#if !TEST_R5
		BEntry entry((const BDirectory*)NULL, "/tmp");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
#endif
	}

	void BadlyInitializedDir_DirPathConstructor_ReturnsOk()
	{
		BDirectory dir(badEntry1.cpath);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		BEntry entry(&dir, "/tmp");
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void Existing_SetToPath_ReturnsExpected()
	{
		_SetToPathTest(dir1, B_OK);
		_SetToPathTest(dir2, B_OK);
		_SetToPathTest(file1, B_OK);
		_SetToPathTest(subDir1, B_OK);
		_SetToPathTest(abstractEntry1, B_OK);

		_SetToPathTest(dir1, B_OK, true);
		_SetToPathTest(dir2, B_OK, true);
		_SetToPathTest(file1, B_OK, true);
		_SetToPathTest(subDir1, B_OK, true);
		_SetToPathTest(abstractEntry1, B_OK, true);
	}

	void Link_SetToPath_ReturnsExpected()
	{
		// don't traverse
		_SetToPathTest(absDirLink1, B_OK);
		_SetToPathTest(absDirLink2, B_OK);
		_SetToPathTest(absDirLink3, B_OK);
		_SetToPathTest(absDirLink4, B_OK);
		_SetToPathTest(relDirLink1, B_OK);
		_SetToPathTest(relDirLink2, B_OK);
		_SetToPathTest(relDirLink3, B_OK);
		_SetToPathTest(relDirLink4, B_OK);
		_SetToPathTest(absFileLink1, B_OK);
		_SetToPathTest(absFileLink2, B_OK);
		_SetToPathTest(absFileLink3, B_OK);
		_SetToPathTest(absFileLink4, B_OK);
		_SetToPathTest(relFileLink1, B_OK);
		_SetToPathTest(relFileLink2, B_OK);
		_SetToPathTest(relFileLink3, B_OK);
		_SetToPathTest(relFileLink4, B_OK);

		// traverse
		_SetToPathTest(absDirLink1, B_OK, true);
		_SetToPathTest(absDirLink2, B_OK, true);
		_SetToPathTest(absDirLink3, B_OK, true);
		_SetToPathTest(absDirLink4, B_OK, true);
		_SetToPathTest(relDirLink1, B_OK, true);
		_SetToPathTest(relDirLink2, B_OK, true);
		_SetToPathTest(relDirLink3, B_OK, true);
		_SetToPathTest(relDirLink4, B_OK, true);
		_SetToPathTest(absFileLink1, B_OK, true);
		_SetToPathTest(absFileLink2, B_OK, true);
		_SetToPathTest(absFileLink3, B_OK, true);
		_SetToPathTest(absFileLink4, B_OK, true);
		_SetToPathTest(relFileLink1, B_OK, true);
		_SetToPathTest(relFileLink2, B_OK, true);
		_SetToPathTest(relFileLink3, B_OK, true);
		_SetToPathTest(relFileLink4, B_OK, true);
	}

	void BadLink_SetToPath_ReturnsExpected()
	{
		_SetToPathTest(badEntry1, B_ENTRY_NOT_FOUND);
		_SetToPathTest(absBadLink1, B_OK);
		_SetToPathTest(absBadLink2, B_OK);
		_SetToPathTest(absBadLink3, B_OK);
		_SetToPathTest(absBadLink4, B_OK);
		_SetToPathTest(relBadLink1, B_OK);
		_SetToPathTest(relBadLink2, B_OK);
		_SetToPathTest(relBadLink3, B_OK);
		_SetToPathTest(relBadLink4, B_OK);
		_SetToPathTest(absVeryBadLink1, B_OK);
		_SetToPathTest(absVeryBadLink2, B_OK);
		_SetToPathTest(absVeryBadLink3, B_OK);
		_SetToPathTest(absVeryBadLink4, B_OK);
		_SetToPathTest(relVeryBadLink1, B_OK);
		_SetToPathTest(relVeryBadLink2, B_OK);
		_SetToPathTest(relVeryBadLink3, B_OK);
		_SetToPathTest(relVeryBadLink4, B_OK);

		// traverse
		_SetToPathTest(badEntry1, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToPathTest(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	}

	void CyclicLink_SetToPath_ReturnsExpected()
	{
		_SetToPathTest(absCyclicLink1, B_OK);
		_SetToPathTest(relCyclicLink1, B_OK);

		_SetToPathTest(absCyclicLink1, B_LINK_LIMIT, true);
		_SetToPathTest(relCyclicLink1, B_LINK_LIMIT, true);
	}

	void TooLong_SetToPath_ReturnsExpected()
	{
		// R5: returns E2BIG instead of B_NAME_TOO_LONG
		_SetToPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
		// R5: returns B_ERROR instead of B_NAME_TOO_LONG
		_SetToPathTest(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG));

		_SetToPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
		_SetToPathTest(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG), true);
	}

	void Root_SetToPath_ReturnsOk()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void Boot_SetToPath_ReturnsOk()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void Null_SetToPath_ReturnsBadValue()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.SetTo((const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
	}

	void Existing_SetToRef_ReturnsExpected()
	{
		_SetToRefTest(dir1, B_OK);
		_SetToRefTest(dir2, B_OK);
		_SetToRefTest(file1, B_OK);
		_SetToRefTest(subDir1, B_OK);
		_SetToRefTest(abstractEntry1, B_OK);

		_SetToRefTest(dir1, B_OK, true);
		_SetToRefTest(dir2, B_OK, true);
		_SetToRefTest(file1, B_OK, true);
		_SetToRefTest(subDir1, B_OK, true);
		_SetToRefTest(abstractEntry1, B_OK, true);
	}

	void Link_SetToRef_ReturnsExpected()
	{
		// don't traverse
		_SetToRefTest(absDirLink1, B_OK);
		_SetToRefTest(absDirLink2, B_OK);
		_SetToRefTest(absDirLink3, B_OK);
		_SetToRefTest(absDirLink4, B_OK);
		_SetToRefTest(relDirLink1, B_OK);
		_SetToRefTest(relDirLink2, B_OK);
		_SetToRefTest(relDirLink3, B_OK);
		_SetToRefTest(relDirLink4, B_OK);
		_SetToRefTest(absFileLink1, B_OK);
		_SetToRefTest(absFileLink2, B_OK);
		_SetToRefTest(absFileLink3, B_OK);
		_SetToRefTest(absFileLink4, B_OK);
		_SetToRefTest(relFileLink1, B_OK);
		_SetToRefTest(relFileLink2, B_OK);
		_SetToRefTest(relFileLink3, B_OK);
		_SetToRefTest(relFileLink4, B_OK);

		// traverse
		_SetToRefTest(absDirLink1, B_OK, true);
		_SetToRefTest(absDirLink2, B_OK, true);
		_SetToRefTest(absDirLink3, B_OK, true);
		_SetToRefTest(absDirLink4, B_OK, true);
		_SetToRefTest(relDirLink1, B_OK, true);
		_SetToRefTest(relDirLink2, B_OK, true);
		_SetToRefTest(relDirLink3, B_OK, true);
		_SetToRefTest(relDirLink4, B_OK, true);
		_SetToRefTest(absFileLink1, B_OK, true);
		_SetToRefTest(absFileLink2, B_OK, true);
		_SetToRefTest(absFileLink3, B_OK, true);
		_SetToRefTest(absFileLink4, B_OK, true);
		_SetToRefTest(relFileLink1, B_OK, true);
		_SetToRefTest(relFileLink2, B_OK, true);
		_SetToRefTest(relFileLink3, B_OK, true);
		_SetToRefTest(relFileLink4, B_OK, true);
	}

	void BadLink_SetToRef_ReturnsExpected()
	{
		_SetToRefTest(absBadLink1, B_OK);
		_SetToRefTest(absBadLink2, B_OK);
		_SetToRefTest(absBadLink3, B_OK);
		_SetToRefTest(absBadLink4, B_OK);
		_SetToRefTest(relBadLink1, B_OK);
		_SetToRefTest(relBadLink2, B_OK);
		_SetToRefTest(relBadLink3, B_OK);
		_SetToRefTest(relBadLink4, B_OK);
		_SetToRefTest(absVeryBadLink1, B_OK);
		_SetToRefTest(absVeryBadLink2, B_OK);
		_SetToRefTest(absVeryBadLink3, B_OK);
		_SetToRefTest(absVeryBadLink4, B_OK);
		_SetToRefTest(relVeryBadLink1, B_OK);
		_SetToRefTest(relVeryBadLink2, B_OK);
		_SetToRefTest(relVeryBadLink3, B_OK);
		_SetToRefTest(relVeryBadLink4, B_OK);

		// traverse
		_SetToRefTest(absBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToRefTest(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	}

	void CyclicLink_SetToRef_ReturnsExpected()
	{
		_SetToRefTest(absCyclicLink1, B_OK);
		_SetToRefTest(relCyclicLink1, B_OK);

		_SetToRefTest(absCyclicLink1, B_LINK_LIMIT, true);
		_SetToRefTest(relCyclicLink1, B_LINK_LIMIT, true);
	}

	void Null_SetToRef_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.SetTo((const entry_ref*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
	}

	void Existing_SetToDirPath_ReturnsExpected()
	{
		_SetToDirPathTest(dir1, B_OK);
		_SetToDirPathTest(dir2, B_OK);
		_SetToDirPathTest(file1, B_OK);
		_SetToDirPathTest(subDir1, B_OK);
		_SetToDirPathTest(abstractEntry1, B_OK);

		_SetToDirPathTest(dir1, B_OK, true);
		_SetToDirPathTest(dir2, B_OK, true);
		_SetToDirPathTest(file1, B_OK, true);
		_SetToDirPathTest(subDir1, B_OK, true);
		_SetToDirPathTest(abstractEntry1, B_OK, true);
	}

	void Link_SetToDirPath_ReturnsExpected()
	{
		// don't traverse
		_SetToDirPathTest(absDirLink1, B_OK);
		_SetToDirPathTest(absDirLink2, B_OK);
		_SetToDirPathTest(absDirLink3, B_OK);
		_SetToDirPathTest(absDirLink4, B_OK);
		_SetToDirPathTest(relDirLink1, B_OK);
		_SetToDirPathTest(relDirLink2, B_OK);
		_SetToDirPathTest(relDirLink3, B_OK);
		_SetToDirPathTest(relDirLink4, B_OK);
		_SetToDirPathTest(absFileLink1, B_OK);
		_SetToDirPathTest(absFileLink2, B_OK);
		_SetToDirPathTest(absFileLink3, B_OK);
		_SetToDirPathTest(absFileLink4, B_OK);
		_SetToDirPathTest(relFileLink1, B_OK);
		_SetToDirPathTest(relFileLink2, B_OK);
		_SetToDirPathTest(relFileLink3, B_OK);
		_SetToDirPathTest(relFileLink4, B_OK);

		// traverse
		_SetToDirPathTest(absDirLink1, B_OK, true);
		_SetToDirPathTest(absDirLink2, B_OK, true);
		_SetToDirPathTest(absDirLink3, B_OK, true);
		_SetToDirPathTest(absDirLink4, B_OK, true);
		_SetToDirPathTest(relDirLink1, B_OK, true);
		_SetToDirPathTest(relDirLink2, B_OK, true);
		_SetToDirPathTest(relDirLink3, B_OK, true);
		_SetToDirPathTest(relDirLink4, B_OK, true);
		_SetToDirPathTest(absFileLink1, B_OK, true);
		_SetToDirPathTest(absFileLink2, B_OK, true);
		_SetToDirPathTest(absFileLink3, B_OK, true);
		_SetToDirPathTest(absFileLink4, B_OK, true);
		_SetToDirPathTest(relFileLink1, B_OK, true);
		_SetToDirPathTest(relFileLink2, B_OK, true);
		_SetToDirPathTest(relFileLink3, B_OK, true);
		_SetToDirPathTest(relFileLink4, B_OK, true);
	}

	void BadLink_SetToDirPath_ReturnsExpected()
	{
		_SetToDirPathTest(absBadLink1, B_OK);
		_SetToDirPathTest(absBadLink2, B_OK);
		_SetToDirPathTest(absBadLink3, B_OK);
		_SetToDirPathTest(absBadLink4, B_OK);
		_SetToDirPathTest(relBadLink1, B_OK);
		_SetToDirPathTest(relBadLink2, B_OK);
		_SetToDirPathTest(relBadLink3, B_OK);
		_SetToDirPathTest(relBadLink4, B_OK);
		_SetToDirPathTest(absVeryBadLink1, B_OK);
		_SetToDirPathTest(absVeryBadLink2, B_OK);
		_SetToDirPathTest(absVeryBadLink3, B_OK);
		_SetToDirPathTest(absVeryBadLink4, B_OK);
		_SetToDirPathTest(relVeryBadLink1, B_OK);
		_SetToDirPathTest(relVeryBadLink2, B_OK);
		_SetToDirPathTest(relVeryBadLink3, B_OK);
		_SetToDirPathTest(relVeryBadLink4, B_OK);

		// traverse
		_SetToDirPathTest(badEntry1, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
		_SetToDirPathTest(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	}

	void CyclicLink_SetToDirPath_ReturnsExpected()
	{
		_SetToDirPathTest(absCyclicLink1, B_OK);
		_SetToDirPathTest(relCyclicLink1, B_OK);

		_SetToDirPathTest(absCyclicLink1, B_LINK_LIMIT, true);
		_SetToDirPathTest(relCyclicLink1, B_LINK_LIMIT, true);
	}

	void TooLong_SetToDirPath_ReturnsExpected()
	{
		// R5: returns E2BIG instead of B_NAME_TOO_LONG
		_SetToDirPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
// Haiku: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
		_SetToDirPathTest(tooLongDir16, B_OK);
#endif

		// R5: returns E2BIG instead of B_NAME_TOO_LONG
		_SetToDirPathTest(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
// Haiku: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
		_SetToDirPathTest(tooLongDir16, B_OK, true);
#endif
	}

	void RootDotLeaf_SetToDirPath_ReturnsOk()
	{
		BDirectory dir("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(&dir, "."));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void RootBootApplicationLeaf_SetToDirPath_ReturnsOk()
	{
		BDirectory dir("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(&dir, "boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void NullLeaf_SetToDirPath_ReturnsOk()
	{
		BDirectory dir("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(&dir, (const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void NullDirAndLeafName_SetToDirPath_ReturnsBadValue()
	{
#if !TEST_R5
		BEntry entry;
		chdir("/");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.SetTo((const BDirectory*)NULL, "tmp"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
		RestoreCWD();
#endif
	}

	void NullDirAndNullLeaf_SetToDirPath_ReturnsBadValue()
	{
#if !TEST_R5
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.SetTo((const BDirectory*)NULL, (const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
#endif
	}

	void NullDirAndAbsolutePath_SetToDirPath_ReturnsBadValue()
	{
#if !TEST_R5
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.SetTo((const BDirectory*)NULL, "/tmp"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.InitCheck());
#endif
	}

	void BadlyInitializedDir_SetToDirPath_ReturnsOk()
	{
		BEntry entry;
		BDirectory dir(badEntry1.cpath);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(&dir, "/tmp"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
	}

	void Initialized_Unset_InitCheckReturnsNoInit()
	{
		BEntry entry(dir1.cpath);
		entry.Unset();
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
	}

	void Uninitialized_Exists_ReturnsFalse()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT(entry.Exists() == false);
	}

	void BadlyInitialized_Exists_ReturnsFalse()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT(entry.Exists() == false);
	}

	void Root_Exists_ReturnsTrue()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT(entry.Exists() == true);
	}

	void Uninitialized_GetPath_ReturnsNoInit()
	{
		BEntry entry;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetPath(&path));
	}

	void BadlyInitialized_GetPath_ReturnsNoInit()
	{
		BEntry entry;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetPath(&path));
	}

	void TooLongPath_GetPath_ReturnsOk()
	{
		BEntry entry;
		BPath path;
		// too long pathname
// Haiku: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
		BDirectory dir(tooLongDir16.super->super->cpath);
		std::string entryName = tooLongDir16.super->name + "/" + tooLongDir16.name;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(&dir, entryName.c_str()));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path == tooLongDir16.cpath);
#endif
	}

	void Uninitialized_GetName_ReturnsNoInit()
	{
		BEntry entry;
		char name[B_FILE_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetName(name));
	}

	void BadlyInitialized_GetName_ReturnsNoInit()
	{
		BEntry entry;
		char name[B_FILE_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetName(name));
	}

	void Uninitialized_GetParentEntry_ReturnsNoInit()
	{
		BEntry entry;
		BEntry parentEntry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetParent(&parentEntry));
	}

	void BadlyInitialized_GetParentEntry_ReturnsNoInit()
	{
		BEntry entry;
		BEntry parentEntry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetParent(&parentEntry));
	}

	void Root_GetParentEntry_ReturnsEntryNotFound()
	{
		BEntry entry;
		BEntry parentEntry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.GetParent(&parentEntry));
	}

	void Uninitialized_GetParentDir_ReturnsNoInit()
	{
		BEntry entry;
		BDirectory parentDir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetParent(&parentDir));
	}

	void BadlyInitialized_GetParentDir_ReturnsNoInit()
	{
		BEntry entry;
		BDirectory parentDir;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetParent(&parentDir));
	}

	void Root_GetParentDir_ReturnsEntryNotFound()
	{
		BEntry entry;
		BDirectory parentDir;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.GetParent(&parentDir));
	}

	void Uninitialized_GetRef_ReturnsNoInit()
	{
		BEntry entry;
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetRef(&ref));
	}

	void BadlyInitialized_GetRef_ReturnsNoInit()
	{
		BEntry entry;
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.GetRef(&ref));
	}

	void Root_GetRef_ReturnsOk()
	{
		BEntry entry;
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
	}

	void SameDir_RenameOnFile_ReturnsExpected()
	{
		_RenameTestFile(&file2, &file2, false, false, B_FILE_EXISTS);
		_RenameTestFile(&file2, &file2, false, true, B_NOT_ALLOWED);
		_RenameTestFile(&file2, &file4, false, false, B_OK);
	}

	void DifferentDir_RenameOnFile_ReturnsExpected() { _RenameTestFile(&file2, &file3, false, false, B_OK); }

	void Clobber_RenameOnFile_ReturnsExpected()
	{
		_RenameTestFile(&file2, &file3, true, true, B_OK);
	}

	void NoClobber_RenameOnFile_ReturnsExpected()
	{
		_RenameTestFile(&file2, &file3, true, false, B_FILE_EXISTS);
	}

	void SameDir_RenameOnDir_ReturnsExpected()
	{
		_RenameTestDir(&file2, &file2, false, false, B_FILE_EXISTS);
		_RenameTestDir(&file2, &file2, false, true, B_NOT_ALLOWED);
		_RenameTestDir(&file2, &file4, false, false, B_OK);
	}

	void DifferentDir_RenameOnDir_ReturnsExpected() { _RenameTestDir(&file2, &file3, false, false, B_OK); }

	void Clobber_RenameOnDir_ReturnsExpected()
	{
		_RenameTestDir(&file2, &file3, true, true, B_OK);
	}

	void NoClobber_RenameOnDir_ReturnsExpected()
	{
		_RenameTestDir(&file2, &file3, true, false, B_FILE_EXISTS);
	}

	void SameDir_RenameOnLink_ReturnsExpected()
	{
		_RenameTestLink(&file2, &file2, false, false, B_FILE_EXISTS);
		_RenameTestLink(&file2, &file2, false, true, B_NOT_ALLOWED);
		_RenameTestLink(&file2, &file3, false, false, B_OK);
	}

	void DifferentDir_RenameOnLink_ReturnsExpected()
	{
		_RenameTestLink(&file2, &file3, false, false, B_OK);
	}

	void Clobber_RenameOnLink_ReturnsExpected()
	{
		_RenameTestLink(&file2, &file3, true, true, B_OK);
	}

	void NoClobber_RenameOnLink_ReturnsExpected()
	{
		_RenameTestLink(&file2, &file3, true, false, B_FILE_EXISTS);
	}

	void File_Rename_DoesNotClobberNotEmptyDir()
	{
		BEntry entry;
		CreateFile(file3.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file3.cpath));
		CPPUNIT_ASSERT_EQUAL(B_DIRECTORY_NOT_EMPTY, entry.Rename(dir1.cpath, true));
		CPPUNIT_ASSERT(PingDir(dir1.cpath));
		CPPUNIT_ASSERT(PingFile(file3.cpath, &entry));
		RemoveFile(file3.cpath);
	}

	void File_Rename_ClobbersEmptyDir()
	{
		BEntry entry;
		CreateFile(file3.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file3.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.Rename(subDir1.cpath, true));
		CPPUNIT_ASSERT(PingFile(subDir1.cpath, &entry));
		CPPUNIT_ASSERT(!PingFile(file3.cpath));
		RemoveFile(subDir1.cpath);
	}

	void Abstract_Rename_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.Rename(file4.cname));
	}

	void Uninitialized_Rename_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.Rename(file4.cname));
	}

	void Root_Rename_Fails()
	{
		BEntry root("/");
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, root.Rename("/", false));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, root.Rename("/", true));
	}

	void Abstract_Rename_Fails()
	{
		BEntry abstract(abstractEntry1.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, abstract.InitCheck());
		CPPUNIT_ASSERT(abstract.Exists() == false);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, abstract.Rename("/boot/DoesntMatter"));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, abstract.Rename("/boot/DontMatter", true));
		CPPUNIT_ASSERT_EQUAL(B_CROSS_DEVICE_LINK, abstract.Rename("/DoesntMatter"));
		CPPUNIT_ASSERT_EQUAL(B_CROSS_DEVICE_LINK, abstract.Rename("/DontMatter", true));
	}

	void Null_Rename_Fails()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file1.cpath));
		CPPUNIT_ASSERT(equals(entry.Rename(NULL, false), B_FILE_EXISTS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(entry.Rename(NULL, true), B_NOT_ALLOWED, B_BAD_VALUE));
	}

	void NullPath_MoveToFile_ReturnsExpected()
	{
		_MoveToTestFile(&file2, file2.super, NULL, false, false, B_FILE_EXISTS);
		_MoveToTestFile(&file2, file2.super, NULL, false, true, B_NOT_ALLOWED);
		_MoveToTestFile(&file2, &dir2, NULL, false, false, B_OK);
		_MoveToTestFile(&file2, &dir2, NULL, true, true, B_OK);
		_MoveToTestFile(&file2, &dir2, NULL, true, false, B_FILE_EXISTS);
	}

	void WithPath_MoveToFile_ReturnsExpected()
	{
		_MoveToTestFile(&file2, file2.super, &file2, false, false, B_FILE_EXISTS);
		_MoveToTestFile(&file2, file2.super, &file2, false, true, B_NOT_ALLOWED);
		_MoveToTestFile(&file2, file2.super, &file3, false, false, B_OK);
		_MoveToTestFile(&file2, &dir2, &file3, false, false, B_OK);
		_MoveToTestFile(&file2, &dir2, &file3, true, true, B_OK);
		_MoveToTestFile(&file2, &dir2, &file3, true, false, B_FILE_EXISTS);
	}

	void NullPath_MoveToDir_ReturnsExpected()
	{
		_MoveToTestDir(&file2, file2.super, NULL, false, false, B_FILE_EXISTS);
		_MoveToTestDir(&file2, file2.super, NULL, false, true, B_NOT_ALLOWED);
		_MoveToTestDir(&file2, &dir2, NULL, false, false, B_OK);
		_MoveToTestDir(&file2, &dir2, NULL, true, true, B_OK);
		_MoveToTestDir(&file2, &dir2, NULL, true, false, B_FILE_EXISTS);
	}

	void WithPath_MoveToDir_ReturnsExpected()
	{
		_MoveToTestDir(&file2, file2.super, &file2, false, false, B_FILE_EXISTS);
		_MoveToTestDir(&file2, file2.super, &file2, false, true, B_NOT_ALLOWED);
		_MoveToTestDir(&file2, file2.super, &file3, false, false, B_OK);
		_MoveToTestDir(&file2, &dir2, &file3, false, false, B_OK);
		_MoveToTestDir(&file2, &dir2, &file3, true, true, B_OK);
		_MoveToTestDir(&file2, &dir2, &file3, true, false, B_FILE_EXISTS);
	}

	void NullPath_MoveToLink_ReturnsExpected()
	{
		_MoveToTestLink(&file2, file2.super, NULL, false, false, B_FILE_EXISTS);
		_MoveToTestLink(&file2, file2.super, NULL, false, true, B_NOT_ALLOWED);
		_MoveToTestLink(&file2, &dir2, NULL, false, false, B_OK);
		_MoveToTestLink(&file2, &dir2, NULL, true, true, B_OK);
		_MoveToTestLink(&file2, &dir2, NULL, true, false, B_FILE_EXISTS);
	}

	void WithPath_MoveToLink_ReturnsExpected()
	{
		_MoveToTestDir(&file2, file2.super, &file2, false, false, B_FILE_EXISTS);
		_MoveToTestDir(&file2, file2.super, &file2, false, true, B_NOT_ALLOWED);
		_MoveToTestDir(&file2, file2.super, &file3, false, false, B_OK);
		_MoveToTestDir(&file2, &dir2, &file3, false, false, B_OK);
		_MoveToTestDir(&file2, &dir2, &file3, true, true, B_OK);
		_MoveToTestDir(&file2, &dir2, &file3, true, false, B_FILE_EXISTS);
	}

	void File_MoveTo_DoesNotClobberNotEmptyDir()
	{
		BDirectory dir;
		BEntry entry;
		CreateFile(file3.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file3.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dir1.super->cpath));
		CPPUNIT_ASSERT_EQUAL(B_DIRECTORY_NOT_EMPTY, entry.MoveTo(&dir, dir1.cname, true));
		CPPUNIT_ASSERT(PingDir(dir1.cpath));
		CPPUNIT_ASSERT(PingFile(file3.cpath, &entry));
		RemoveFile(file3.cpath);
	}

	void File_MoveTo_ClobbersEmptyDir()
	{
		BDirectory dir;
		BEntry entry;
		CreateFile(file3.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file3.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(subDir1.super->cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.MoveTo(&dir, subDir1.cname, true));
		CPPUNIT_ASSERT(PingFile(subDir1.cpath, &entry));
		CPPUNIT_ASSERT(!PingFile(file3.cpath));
		RemoveFile(subDir1.cpath);
	}

	void Abstract_MoveTo_ReturnsExpected()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dir2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.MoveTo(&dir));
	}

	void Uninitialized_MoveTo_ReturnsExpected()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dir2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.MoveTo(&dir));
	}

	void BadInit_MoveTo_ReturnsExpected()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dir2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.MoveTo(&dir));
	}

	void NullDir_MoveTo_ReturnsBadValue()
	{
#if !TEST_R5 // R5 crashes
		BEntry entry(file2.cpath);
		CreateFile(file2.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.MoveTo(NULL, file3.cpath));
		RemoveFile(file3.cpath);
#endif
	}

	void UninitializedDir_MoveTo_ReturnsBadValue()
	{
		BEntry entry;
		BDirectory dir;
		CreateFile(file2.cpath);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file2.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, entry.MoveTo(&dir, file3.cpath));
		RemoveFile(file3.cpath);
	}

	void File_Remove_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(file1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.Remove());
		CPPUNIT_ASSERT(!PingFile(file1.cpath));
	}

	void EmptyDir_Remove_ReturnsOk()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(subDir1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.Remove());
		CPPUNIT_ASSERT(!PingDir(subDir1.cpath));
	}

	void NonEmptyDir_Remove_ReturnsDirectoryNotEmpty()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(dir1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_DIRECTORY_NOT_EMPTY, entry.Remove());
		CPPUNIT_ASSERT(PingDir(dir1.cpath));
	}

	void Link_Remove_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(absFileLink1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.Remove());
		CPPUNIT_ASSERT(!PingLink(absFileLink1.cpath));
	}

	void Abstract_Remove_ReturnsEntryNotFound()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(abstractEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.Remove());
	}

	void Uninitialized_Remove_ReturnsNoInit()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.Remove());
	}

	void BadlyInitialized_Remove_ReturnsNoInit()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, entry.SetTo(badEntry1.cpath));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.Remove());
	}

	void Uninitialized_Comparison_ReturnsExpected()
	{
		BEntry entry;
		BEntry entry2;
		CPPUNIT_ASSERT(entry == entry2);
		CPPUNIT_ASSERT(entry2 == entry);
		CPPUNIT_ASSERT(!(entry != entry2));
		CPPUNIT_ASSERT(!(entry2 != entry));
	}

	void InitializedAndUninitialized_Comparison_ReturnsExpected()
	{
		BEntry entry(file1.cpath);
		BEntry entry2;
		CPPUNIT_ASSERT(!(entry == entry2));
		CPPUNIT_ASSERT(!(entry2 == entry));
		CPPUNIT_ASSERT(entry != entry2);
		CPPUNIT_ASSERT(entry2 != entry);
	}

	void UninitializedAndInitialized_Comparison_ReturnsExpected()
	{
		BEntry entry;
		BEntry entry2(file1.cpath);
		CPPUNIT_ASSERT(!(entry == entry2));
		CPPUNIT_ASSERT(!(entry2 == entry));
		CPPUNIT_ASSERT(entry != entry2);
		CPPUNIT_ASSERT(entry2 != entry);
	}

	void Initialized_Comparison_ReturnsExpected()
	{
		TestEntry* testEntries[] = {&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
			&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4, &relDirLink1, &relDirLink2,
			&relDirLink3, &relDirLink4, &absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
			&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4, &absBadLink1, &absBadLink2,
			&absBadLink3, &absBadLink4, &relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
			&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
			&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4};
		int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
		for (int32 i = 0; i < testEntryCount; i++) {
			TestEntry* testEntry = testEntries[i];
			TestEntry* traversedTestEntry = resolve_link(testEntry);
			BEntry entry(testEntry->cpath);
			BEntry traversedEntry(testEntry->cpath, true);
			for (int32 k = 0; k < testEntryCount; k++) {
				TestEntry* testEntry2 = testEntries[k];
				TestEntry* traversedTestEntry2 = resolve_link(testEntry2);
				BEntry entry2(testEntry2->cpath);
				BEntry traversedEntry2(testEntry2->cpath, true);
				compareEntries(entry, entry2, testEntry, testEntry2, false, false);
				compareEntries(traversedEntry, entry2, traversedTestEntry, testEntry2, true, false);
				compareEntries(entry, traversedEntry2, testEntry, traversedTestEntry2, false, true);
				compareEntries(traversedEntry, traversedEntry2, traversedTestEntry,
					traversedTestEntry2, true, true);
			}
		}
	}

	void Uninitialized_CopyConstructor_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		BEntry entry2(entry);
#if !TEST_R5 // R5 returns B_BAD_VALUE
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry2.InitCheck());
#endif
		CPPUNIT_ASSERT(entry == entry2);
	}

	void Initialized_CopyConstructor_ReturnsExpected()
	{
		TestEntry* testEntries[] = {&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
			&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4, &relDirLink1, &relDirLink2,
			&relDirLink3, &relDirLink4, &absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
			&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4, &absBadLink1, &absBadLink2,
			&absBadLink3, &absBadLink4, &relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
			&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
			&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4};
		int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
		for (int32 i = 0; i < testEntryCount; i++) {
			TestEntry* testEntry = testEntries[i];
			BEntry entry(testEntry->cpath);
			BEntry entry2(entry);
			CPPUNIT_ASSERT(entry == entry2);
			CPPUNIT_ASSERT(entry2 == entry);
			CPPUNIT_ASSERT(!(entry != entry2));
			CPPUNIT_ASSERT(!(entry2 != entry));
		}
	}

	void Uninitialized_Assignment_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		BEntry entry2;
		entry2 = entry;
#if !TEST_R5 // R5 returns B_BAD_VALUE
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry2.InitCheck());
#endif
		CPPUNIT_ASSERT(entry == entry2);
	}

	void Initialized_Assignment_ReturnsExpected()
	{
		TestEntry* testEntries[] = {&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
			&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4, &relDirLink1, &relDirLink2,
			&relDirLink3, &relDirLink4, &absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
			&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4, &absBadLink1, &absBadLink2,
			&absBadLink3, &absBadLink4, &relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
			&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
			&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4};
		int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
		for (int32 i = 0; i < testEntryCount; i++) {
			TestEntry* testEntry = testEntries[i];
			BEntry entry(testEntry->cpath);
			BEntry entry2;
			BEntry entry3(file1.cpath);
			entry2 = entry;
			entry3 = entry;
			CPPUNIT_ASSERT(entry == entry2);
			CPPUNIT_ASSERT(entry2 == entry);
			CPPUNIT_ASSERT(!(entry != entry2));
			CPPUNIT_ASSERT(!(entry2 != entry));
			CPPUNIT_ASSERT(entry == entry3);
			CPPUNIT_ASSERT(entry3 == entry);
			CPPUNIT_ASSERT(!(entry != entry3));
			CPPUNIT_ASSERT(!(entry3 != entry));
		}
	}

	void InitializedAndUninitialized_Assignment_ReturnsExpected()
	{
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		BEntry entry2(file1.cpath);
		entry2 = entry;
#if !TEST_R5 // R5 returns B_BAD_VALUE
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry2.InitCheck());
#endif
		CPPUNIT_ASSERT(entry == entry2);
	}

	void LockedNode_Exists_ReturnsTrue()
	{
		BNode node(file1.cpath);
		BEntry entry(file1.cpath);

		CPPUNIT_ASSERT_EQUAL(B_OK, node.Lock());
		CPPUNIT_ASSERT(entry.Exists());
	}

	void EntryRef_GetRefForPath_ReturnsExpected()
	{
		TestEntry* testEntries[] = {&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
			&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4, &relDirLink1, &relDirLink2,
			&relDirLink3, &relDirLink4, &absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
			&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4, &absBadLink1, &absBadLink2,
			&absBadLink3, &absBadLink4, &relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
			&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
			&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4};
		int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
		for (int32 i = 0; i < testEntryCount; i++) {
			TestEntry* testEntry = testEntries[i];
			const char* path = testEntry->cpath;
			entry_ref ref;
			if (testEntry->isBad()) {
				CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, get_ref_for_path(path, &ref));
			} else {
				CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(path, &ref));
				const entry_ref& testEntryRef = testEntry->get_ref();
				CPPUNIT_ASSERT(testEntryRef.device == ref.device);
				CPPUNIT_ASSERT(testEntryRef.directory == ref.directory);
				CPPUNIT_ASSERT(strcmp(testEntryRef.name, ref.name) == 0);
				CPPUNIT_ASSERT(testEntryRef == ref);
				CPPUNIT_ASSERT(!(testEntryRef != ref));
				CPPUNIT_ASSERT(ref == testEntryRef);
				CPPUNIT_ASSERT(!(ref != testEntryRef));
			}
		}
	}

	void Roots_GetRefForPath_ReturnsExpected()
	{
		entry_ref ref, ref2;
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path("/", &ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, get_entry_ref_for_entry("/", ".", &ref2));
		CPPUNIT_ASSERT(ref.device == ref2.device);
		CPPUNIT_ASSERT(ref.directory == ref2.directory);
		CPPUNIT_ASSERT(strcmp(ref.name, ref2.name) == 0);
		CPPUNIT_ASSERT(ref == ref2);
		// fs root dir
		CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path("/boot", &ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, get_entry_ref_for_entry("/", "boot", &ref2));
		CPPUNIT_ASSERT(ref.device == ref2.device);
		CPPUNIT_ASSERT(ref.directory == ref2.directory);
		CPPUNIT_ASSERT(strcmp(ref.name, ref2.name) == 0);
		CPPUNIT_ASSERT(ref == ref2);
		
	}

	void Uninitialized_GetRefForPath_ReturnsExpected()
	{
		entry_ref ref = entry_ref();
		entry_ref ref2 = entry_ref();
		CPPUNIT_ASSERT(ref == ref2);
		CPPUNIT_ASSERT(!(ref != ref2));
		CPPUNIT_ASSERT(!(ref < ref2));
		CPPUNIT_ASSERT(!(ref2 < ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, get_entry_ref_for_entry("/", ".", &ref2));
		CPPUNIT_ASSERT(!(ref == ref2));
		CPPUNIT_ASSERT(ref != ref2);
		CPPUNIT_ASSERT(ref < ref2);
		CPPUNIT_ASSERT(!(ref2 < ref));
	}

	void EntryRef_GetRefForPath_ComparisonsReturnExpected()
	{
		TestEntry* testEntries[] = {&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
			&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4, &relDirLink1, &relDirLink2,
			&relDirLink3, &relDirLink4, &absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
			&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4, &absBadLink1, &absBadLink2,
			&absBadLink3, &absBadLink4, &relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
			&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
			&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4};
		int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
		for (int32 i = 0; i < testEntryCount; i++) {
			TestEntry* testEntry = testEntries[i];
			if (!testEntry->isBad()) {
				entry_ref ref;
				CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testEntry->cpath, &ref));
				for (int32 k = 0; k < testEntryCount; k++) {
					TestEntry* testEntry2 = testEntries[k];
					if (!testEntry2->isBad()) {
						entry_ref ref2;
						CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(testEntry2->cpath, &ref2));
						int cmp = 0;
						if (ref > ref2)
							cmp = 1;
						else if (ref2 > ref)
							cmp = -1;
						CPPUNIT_ASSERT((ref == ref2) == (cmp == 0));
						CPPUNIT_ASSERT((ref2 == ref) == (cmp == 0));
						CPPUNIT_ASSERT((ref != ref2) == (cmp != 0));
						CPPUNIT_ASSERT((ref2 != ref) == (cmp != 0));
						CPPUNIT_ASSERT((ref < ref2) == (cmp < 0));
						CPPUNIT_ASSERT((ref2 < ref) == (cmp > 0));
					}
				}
			}
		}
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(EntryTest, getTestSuiteName());
