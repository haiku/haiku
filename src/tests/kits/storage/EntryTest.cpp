// EntryTest.cpp

#include <errno.h>
#include <list>
#include <map>
#include <set>
#include <stdio.h>
#include <unistd.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <Entry.h>
#include <Directory.h>
#include <Path.h>

#include "EntryTest.h"

enum test_entry_kind {
	DIR_ENTRY,
	FILE_ENTRY,
	LINK_ENTRY,
	ABSTRACT_ENTRY,
	BAD_ENTRY,
};

struct TestEntry {
	TestEntry();

	void init(TestEntry &super, string name, test_entry_kind kind,
			  bool relative = false);
	void initDir(TestEntry &super, string name);
	void initFile(TestEntry &super, string name);
	void initRLink(TestEntry &super, string name, TestEntry &target);
	void initALink(TestEntry &super, string name, TestEntry &target);
	void initPath(const char *pathName = NULL);
	void completeInit();
	bool isConcrete() const { return !(isAbstract() || isBad()); }
	bool isAbstract() const { return kind == ABSTRACT_ENTRY; }
	bool isBad() const { return kind == BAD_ENTRY; }
	const entry_ref &get_ref();

	TestEntry *		super;
	string			name;
	test_entry_kind	kind;
	string			path;
	TestEntry *		target;
	string			link;
	entry_ref		ref;
	bool			relative;
	// C strings
	const char *	cname;
	const char *	cpath;
	const char *	clink;
};

static list<TestEntry*> allTestEntries;

static TestEntry badTestEntry;
static TestEntry testDir;
static TestEntry dir1;
static TestEntry dir2;
static TestEntry file1;
static TestEntry file2;
static TestEntry file3;
static TestEntry file4;
static TestEntry subDir1;
static TestEntry abstractEntry1;
static TestEntry badEntry1;
static TestEntry absDirLink1;
static TestEntry absDirLink2;
static TestEntry absDirLink3;
static TestEntry absDirLink4;
static TestEntry relDirLink1;
static TestEntry relDirLink2;
static TestEntry relDirLink3;
static TestEntry relDirLink4;
static TestEntry absFileLink1;
static TestEntry absFileLink2;
static TestEntry absFileLink3;
static TestEntry absFileLink4;
static TestEntry relFileLink1;
static TestEntry relFileLink2;
static TestEntry relFileLink3;
static TestEntry relFileLink4;
static TestEntry absCyclicLink1;
static TestEntry absCyclicLink2;
static TestEntry relCyclicLink1;
static TestEntry relCyclicLink2;
static TestEntry absBadLink1;
static TestEntry absBadLink2;
static TestEntry absBadLink3;
static TestEntry absBadLink4;
static TestEntry relBadLink1;
static TestEntry relBadLink2;
static TestEntry relBadLink3;
static TestEntry relBadLink4;
static TestEntry absVeryBadLink1;
static TestEntry absVeryBadLink2;
static TestEntry absVeryBadLink3;
static TestEntry absVeryBadLink4;
static TestEntry relVeryBadLink1;
static TestEntry relVeryBadLink2;
static TestEntry relVeryBadLink3;
static TestEntry relVeryBadLink4;
static TestEntry tooLongEntry1;
static TestEntry tooLongDir1;
static TestEntry tooLongDir2;
static TestEntry tooLongDir3;
static TestEntry tooLongDir4;
static TestEntry tooLongDir5;
static TestEntry tooLongDir6;
static TestEntry tooLongDir7;
static TestEntry tooLongDir8;
static TestEntry tooLongDir9;
static TestEntry tooLongDir10;
static TestEntry tooLongDir11;
static TestEntry tooLongDir12;
static TestEntry tooLongDir13;
static TestEntry tooLongDir14;
static TestEntry tooLongDir15;
static TestEntry tooLongDir16;

static string setUpCommandLine;
static string tearDownCommandLine;

// forward declarations
static TestEntry *resolve_link(TestEntry *entry);
static string get_shortest_relative_path(TestEntry *dir, TestEntry *entry);

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
	B_FILE_NOT_FOUND,
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

// get_error_index
static
int32
get_error_index(status_t error)
{
	int32 result = -1;
	for (int32 i = 0; result == -1 && i < kErrorCount; i++) {
		if (kErrors[i] == error)
			result = i;
	}
	if (result == -1)
		printf("WARNING: error %lx is not in the list of errors\n", error);
	return result;
}

// fuzzy_error
static
status_t
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

// fuzzy_equals
static
bool
fuzzy_equals(status_t error, status_t fuzzyError)
{
	bool result = false;
	if (fuzzyError <= 0)
		result = (error == fuzzyError);
	else {
		// decode the error
		int32 index1 = (fuzzyError - 1) / kErrorCount;
		int32 index2 = (fuzzyError - 1) % kErrorCount;
		if (index1 >= kErrorCount)
			printf("WARNING: bad fuzzy error: %lx\n", fuzzyError);
		else {
			status_t error1 = kErrors[index1];
			status_t error2 = kErrors[index2];
			result = (error == error1 || error == error2);
		}
	}
	return result;
}


// Suite
CppUnit::Test*
EntryTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<EntryTest> TC;

	StatableTest::AddBaseClassTests<EntryTest>("BEntry::", suite);

	suite->addTest( new TC("BEntry::Init Test1", &EntryTest::InitTest1) );
	suite->addTest( new TC("BEntry::Init Test2", &EntryTest::InitTest2) );
	suite->addTest( new TC("BEntry::Special cases for Exists(), GetPath(),...",
						   &EntryTest::SpecialGetCasesTest) );
	suite->addTest( new TC("BEntry::Rename Test", &EntryTest::RenameTest) );
	suite->addTest( new TC("BEntry::MoveTo Test", &EntryTest::MoveToTest) );
	suite->addTest( new TC("BEntry::Remove Test", &EntryTest::RemoveTest) );
	suite->addTest( new TC("BEntry::Comparison Test",
						   &EntryTest::ComparisonTest) );
	suite->addTest( new TC("BEntry::Assignment Test",
						   &EntryTest::AssignmentTest) );
	suite->addTest( new TC("BEntry::C Functions Test",
						   &EntryTest::CFunctionsTest) );
//	suite->addTest( new TC("BEntry::Miscellaneous Test", &EntryTest::MiscTest) );

	return suite;
}		

// CreateROStatables
void
EntryTest::CreateROStatables(TestStatables& testEntries)
{
	CreateRWStatables(testEntries);
}

// CreateRWStatables
void
EntryTest::CreateRWStatables(TestStatables& testStatables)
{
	TestEntry *testEntries[] = {
		&dir1, &dir2, &file1, &subDir1,
		&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4,
		&relDirLink1, &relDirLink2, &relDirLink3, &relDirLink4,
		&absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
		&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4,
		&absBadLink1, &absBadLink2, &absBadLink3, &absBadLink4,
		&relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
		&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
		&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4
	};
	int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
	for (int32 i = 0; i < testEntryCount; i++) {
		TestEntry *testEntry = testEntries[i];
		const char *filename = testEntry->cpath;
		testStatables.add(new BEntry(filename), filename);
	}
}

// CreateUninitializedStatables
void
EntryTest::CreateUninitializedStatables(TestStatables& testEntries)
{
	testEntries.add(new BEntry, "");
}

// setUp
void
EntryTest::setUp()
{
	StatableTest::setUp();
	execCommand(setUpCommandLine);
}
	
// tearDown
void
EntryTest::tearDown()
{
	StatableTest::tearDown();
	execCommand(tearDownCommandLine);
}

// examine_entry
static
void
examine_entry(BEntry &entry, TestEntry *testEntry, bool traverse)
{
	if (traverse)
		testEntry = resolve_link(testEntry);
	// Exists()
	CPPUNIT_ASSERT( entry.Exists() == testEntry->isConcrete() );
	// GetPath()
	BPath path;
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == testEntry->cpath );
	// GetName()
	char name[B_FILE_NAME_LENGTH + 1];
	CPPUNIT_ASSERT( entry.GetName(name) == B_OK );
	CPPUNIT_ASSERT( testEntry->name == name );
	// GetParent(BEntry *)
	BEntry parentEntry;
	CPPUNIT_ASSERT( entry.GetParent(&parentEntry) == B_OK );
	CPPUNIT_ASSERT( parentEntry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( parentEntry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == testEntry->super->cpath );
	parentEntry.Unset();
	path.Unset();
	// GetParent(BDirectory *)
	BDirectory parentDir;
	CPPUNIT_ASSERT( entry.GetParent(&parentDir) == B_OK );
	CPPUNIT_ASSERT( parentDir.GetEntry(&parentEntry) == B_OK );
	CPPUNIT_ASSERT( parentEntry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( parentEntry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == testEntry->super->cpath );
	// GetRef()
	entry_ref ref;
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	// We can't get a ref of an entry with a too long path name yet.
	if (testEntry->path.length() < B_PATH_NAME_LENGTH)
		CPPUNIT_ASSERT( ref == testEntry->get_ref() );
}

// InitTest1Paths
void
EntryTest::InitTest1Paths(TestEntry &_testEntry, status_t error, bool traverse)
{
	TestEntry *testEntry = &_testEntry;
	// absolute path
	NextSubTest();
	{
//printf("%s\n", testEntry->cpath);
		BEntry entry(testEntry->cpath, traverse);
		status_t result = entry.InitCheck();
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
		CPPUNIT_ASSERT( fuzzy_equals(result, error) );
		if (result == B_OK)
			examine_entry(entry, testEntry, traverse);
	}
	// relative path
	NextSubTest();
	{
//printf("%s\n", testEntry->cpath);
		if (chdir(testEntry->super->cpath) == 0) {
			BEntry entry(testEntry->cname, traverse);
			status_t result = entry.InitCheck();
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			if (error == B_OK)
				examine_entry(entry, testEntry, traverse);
			RestoreCWD();
		}
	}
}

// InitTest1Refs
void
EntryTest::InitTest1Refs(TestEntry &_testEntry, status_t error, bool traverse)
{
	TestEntry *testEntry = &_testEntry;
	// absolute path
	NextSubTest();
	{
//printf("%s\n", testEntry->cpath);
		BEntry entry(&testEntry->get_ref(), traverse);
		status_t result = entry.InitCheck();
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
		CPPUNIT_ASSERT( fuzzy_equals(result, error) );
		if (error == B_OK)
			examine_entry(entry, testEntry, traverse);
	}
}

// InitTest1DirPaths
void
EntryTest::InitTest1DirPaths(TestEntry &_testEntry, status_t error,
							 bool traverse)
{
	TestEntry *testEntry = &_testEntry;
	// absolute path
	NextSubTest();
	{
		if (!testEntry->isBad()
			&& testEntry->path.length() < B_PATH_NAME_LENGTH) {
//printf("%s\n", testEntry->cpath);
			BDirectory dir("/boot/home/Desktop");
			CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
			BEntry entry(&dir, testEntry->cpath, traverse);
		status_t result = entry.InitCheck();
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
		CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			if (error == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}
	// relative path (one level)
	NextSubTest();
	{
		if (!testEntry->isBad()
			&& testEntry->super->path.length() < B_PATH_NAME_LENGTH) {
//printf("%s + %s\n", testEntry->super->cpath, testEntry->cname);
			BDirectory dir(testEntry->super->cpath);
			CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
			BEntry entry(&dir, testEntry->cname, traverse);
			status_t result = entry.InitCheck();
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			if (error == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}
	// relative path (two levels)
	NextSubTest();
	{
		if (!testEntry->super->isBad() && !testEntry->super->super->isBad()) {
			string entryName  = testEntry->super->name + "/" + testEntry->name;
//printf("%s + %s\n", testEntry->super->super->cpath, entryName.c_str());
			BDirectory dir(testEntry->super->super->cpath);
			CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
			BEntry entry(&dir, entryName.c_str(), traverse);
			status_t result = entry.InitCheck();
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			if (error == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}
}

// InitTest1
void
EntryTest::InitTest1()
{
	// 1. default constructor
	NextSubTest();
	{
		BEntry entry;
		CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	}

	// 2. BEntry(const char *, bool)
	// don't traverse
	InitTest1Paths(dir1, B_OK);
	InitTest1Paths(dir2, B_OK);
	InitTest1Paths(file1, B_OK);
	InitTest1Paths(subDir1, B_OK);
	InitTest1Paths(abstractEntry1, B_OK);
	InitTest1Paths(badEntry1, B_ENTRY_NOT_FOUND);
	InitTest1Paths(absDirLink1, B_OK);
	InitTest1Paths(absDirLink2, B_OK);
	InitTest1Paths(absDirLink3, B_OK);
	InitTest1Paths(absDirLink4, B_OK);
	InitTest1Paths(relDirLink1, B_OK);
	InitTest1Paths(relDirLink2, B_OK);
	InitTest1Paths(relDirLink3, B_OK);
	InitTest1Paths(relDirLink4, B_OK);
	InitTest1Paths(absFileLink1, B_OK);
	InitTest1Paths(absFileLink2, B_OK);
	InitTest1Paths(absFileLink3, B_OK);
	InitTest1Paths(absFileLink4, B_OK);
	InitTest1Paths(relFileLink1, B_OK);
	InitTest1Paths(relFileLink2, B_OK);
	InitTest1Paths(relFileLink3, B_OK);
	InitTest1Paths(relFileLink4, B_OK);
	InitTest1Paths(absCyclicLink1, B_OK);
	InitTest1Paths(relCyclicLink1, B_OK);
	InitTest1Paths(absBadLink1, B_OK);
	InitTest1Paths(absBadLink2, B_OK);
	InitTest1Paths(absBadLink3, B_OK);
	InitTest1Paths(absBadLink4, B_OK);
	InitTest1Paths(relBadLink1, B_OK);
	InitTest1Paths(relBadLink2, B_OK);
	InitTest1Paths(relBadLink3, B_OK);
	InitTest1Paths(relBadLink4, B_OK);
	InitTest1Paths(absVeryBadLink1, B_OK);
	InitTest1Paths(absVeryBadLink2, B_OK);
	InitTest1Paths(absVeryBadLink3, B_OK);
	InitTest1Paths(absVeryBadLink4, B_OK);
	InitTest1Paths(relVeryBadLink1, B_OK);
	InitTest1Paths(relVeryBadLink2, B_OK);
	InitTest1Paths(relVeryBadLink3, B_OK);
	InitTest1Paths(relVeryBadLink4, B_OK);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest1Paths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
// R5: returns B_ERROR instead of B_NAME_TOO_LONG
	InitTest1Paths(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG));
	// traverse
	InitTest1Paths(dir1, B_OK, true);
	InitTest1Paths(dir2, B_OK, true);
	InitTest1Paths(file1, B_OK, true);
	InitTest1Paths(subDir1, B_OK, true);
	InitTest1Paths(abstractEntry1, B_OK, true);
	InitTest1Paths(badEntry1, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absDirLink1, B_OK, true);
	InitTest1Paths(absDirLink2, B_OK, true);
	InitTest1Paths(absDirLink3, B_OK, true);
	InitTest1Paths(absDirLink4, B_OK, true);
	InitTest1Paths(relDirLink1, B_OK, true);
	InitTest1Paths(relDirLink2, B_OK, true);
	InitTest1Paths(relDirLink3, B_OK, true);
	InitTest1Paths(relDirLink4, B_OK, true);
	InitTest1Paths(absFileLink1, B_OK, true);
	InitTest1Paths(absFileLink2, B_OK, true);
	InitTest1Paths(absFileLink3, B_OK, true);
	InitTest1Paths(absFileLink4, B_OK, true);
	InitTest1Paths(relFileLink1, B_OK, true);
	InitTest1Paths(relFileLink2, B_OK, true);
	InitTest1Paths(relFileLink3, B_OK, true);
	InitTest1Paths(relFileLink4, B_OK, true);
	InitTest1Paths(absCyclicLink1, B_LINK_LIMIT, true);
	InitTest1Paths(relCyclicLink1, B_LINK_LIMIT, true);
	InitTest1Paths(absBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Paths(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest1Paths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
// R5: returns B_ERROR instead of B_NAME_TOO_LONG
	InitTest1Paths(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG), true);

	// special cases (root dir)
	NextSubTest();
	{
		BEntry entry("/");
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// special cases (fs root dir)
	NextSubTest();
	{
		BEntry entry("/boot");
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// bad args
	NextSubTest();
	{
		BEntry entry((const char*)NULL);
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
	}

	// 3. BEntry(const entry_ref *, bool)
	// don't traverse
	InitTest1Refs(dir1, B_OK);
	InitTest1Refs(dir2, B_OK);
	InitTest1Refs(file1, B_OK);
	InitTest1Refs(subDir1, B_OK);
	InitTest1Refs(abstractEntry1, B_OK);
	InitTest1Refs(absDirLink1, B_OK);
	InitTest1Refs(absDirLink2, B_OK);
	InitTest1Refs(absDirLink3, B_OK);
	InitTest1Refs(absDirLink4, B_OK);
	InitTest1Refs(relDirLink1, B_OK);
	InitTest1Refs(relDirLink2, B_OK);
	InitTest1Refs(relDirLink3, B_OK);
	InitTest1Refs(relDirLink4, B_OK);
	InitTest1Refs(absFileLink1, B_OK);
	InitTest1Refs(absFileLink2, B_OK);
	InitTest1Refs(absFileLink3, B_OK);
	InitTest1Refs(absFileLink4, B_OK);
	InitTest1Refs(relFileLink1, B_OK);
	InitTest1Refs(relFileLink2, B_OK);
	InitTest1Refs(relFileLink3, B_OK);
	InitTest1Refs(relFileLink4, B_OK);
	InitTest1Refs(absCyclicLink1, B_OK);
	InitTest1Refs(relCyclicLink1, B_OK);
	InitTest1Refs(absBadLink1, B_OK);
	InitTest1Refs(absBadLink2, B_OK);
	InitTest1Refs(absBadLink3, B_OK);
	InitTest1Refs(absBadLink4, B_OK);
	InitTest1Refs(relBadLink1, B_OK);
	InitTest1Refs(relBadLink2, B_OK);
	InitTest1Refs(relBadLink3, B_OK);
	InitTest1Refs(relBadLink4, B_OK);
	InitTest1Refs(absVeryBadLink1, B_OK);
	InitTest1Refs(absVeryBadLink2, B_OK);
	InitTest1Refs(absVeryBadLink3, B_OK);
	InitTest1Refs(absVeryBadLink4, B_OK);
	InitTest1Refs(relVeryBadLink1, B_OK);
	InitTest1Refs(relVeryBadLink2, B_OK);
	InitTest1Refs(relVeryBadLink3, B_OK);
	InitTest1Refs(relVeryBadLink4, B_OK);
	// traverse
	InitTest1Refs(dir1, B_OK, true);
	InitTest1Refs(dir2, B_OK, true);
	InitTest1Refs(file1, B_OK, true);
	InitTest1Refs(subDir1, B_OK, true);
	InitTest1Refs(abstractEntry1, B_OK, true);
	InitTest1Refs(absDirLink1, B_OK, true);
	InitTest1Refs(absDirLink2, B_OK, true);
	InitTest1Refs(absDirLink3, B_OK, true);
	InitTest1Refs(absDirLink4, B_OK, true);
	InitTest1Refs(relDirLink1, B_OK, true);
	InitTest1Refs(relDirLink2, B_OK, true);
	InitTest1Refs(relDirLink3, B_OK, true);
	InitTest1Refs(relDirLink4, B_OK, true);
	InitTest1Refs(absFileLink1, B_OK, true);
	InitTest1Refs(absFileLink2, B_OK, true);
	InitTest1Refs(absFileLink3, B_OK, true);
	InitTest1Refs(absFileLink4, B_OK, true);
	InitTest1Refs(relFileLink1, B_OK, true);
	InitTest1Refs(relFileLink2, B_OK, true);
	InitTest1Refs(relFileLink3, B_OK, true);
	InitTest1Refs(relFileLink4, B_OK, true);
	InitTest1Refs(absCyclicLink1, B_LINK_LIMIT, true);
	InitTest1Refs(relCyclicLink1, B_LINK_LIMIT, true);
	InitTest1Refs(absBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1Refs(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	// bad args
	NextSubTest();
	{
		BEntry entry((const entry_ref*)NULL);
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
	}

	// 4. BEntry(const BDirectory *, const char *, bool)
	// don't traverse
	InitTest1DirPaths(dir1, B_OK);
	InitTest1DirPaths(dir2, B_OK);
	InitTest1DirPaths(file1, B_OK);
	InitTest1DirPaths(subDir1, B_OK);
	InitTest1DirPaths(abstractEntry1, B_OK);
	InitTest1DirPaths(badEntry1, B_ENTRY_NOT_FOUND);
	InitTest1DirPaths(absDirLink1, B_OK);
	InitTest1DirPaths(absDirLink2, B_OK);
	InitTest1DirPaths(absDirLink3, B_OK);
	InitTest1DirPaths(absDirLink4, B_OK);
	InitTest1DirPaths(relDirLink1, B_OK);
	InitTest1DirPaths(relDirLink2, B_OK);
	InitTest1DirPaths(relDirLink3, B_OK);
	InitTest1DirPaths(relDirLink4, B_OK);
	InitTest1DirPaths(absFileLink1, B_OK);
	InitTest1DirPaths(absFileLink2, B_OK);
	InitTest1DirPaths(absFileLink3, B_OK);
	InitTest1DirPaths(absFileLink4, B_OK);
	InitTest1DirPaths(relFileLink1, B_OK);
	InitTest1DirPaths(relFileLink2, B_OK);
	InitTest1DirPaths(relFileLink3, B_OK);
	InitTest1DirPaths(relFileLink4, B_OK);
	InitTest1DirPaths(absCyclicLink1, B_OK);
	InitTest1DirPaths(relCyclicLink1, B_OK);
	InitTest1DirPaths(absBadLink1, B_OK);
	InitTest1DirPaths(absBadLink2, B_OK);
	InitTest1DirPaths(absBadLink3, B_OK);
	InitTest1DirPaths(absBadLink4, B_OK);
	InitTest1DirPaths(relBadLink1, B_OK);
	InitTest1DirPaths(relBadLink2, B_OK);
	InitTest1DirPaths(relBadLink3, B_OK);
	InitTest1DirPaths(relBadLink4, B_OK);
	InitTest1DirPaths(absVeryBadLink1, B_OK);
	InitTest1DirPaths(absVeryBadLink2, B_OK);
	InitTest1DirPaths(absVeryBadLink3, B_OK);
	InitTest1DirPaths(absVeryBadLink4, B_OK);
	InitTest1DirPaths(relVeryBadLink1, B_OK);
	InitTest1DirPaths(relVeryBadLink2, B_OK);
	InitTest1DirPaths(relVeryBadLink3, B_OK);
	InitTest1DirPaths(relVeryBadLink4, B_OK);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest1DirPaths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
// OBOS: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
	InitTest1DirPaths(tooLongDir16, B_OK, true);
#endif
	// traverse
	InitTest1DirPaths(dir1, B_OK, true);
	InitTest1DirPaths(dir2, B_OK, true);
	InitTest1DirPaths(file1, B_OK, true);
	InitTest1DirPaths(subDir1, B_OK, true);
	InitTest1DirPaths(abstractEntry1, B_OK, true);
	InitTest1DirPaths(badEntry1, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absDirLink1, B_OK, true);
	InitTest1DirPaths(absDirLink2, B_OK, true);
	InitTest1DirPaths(absDirLink3, B_OK, true);
	InitTest1DirPaths(absDirLink4, B_OK, true);
	InitTest1DirPaths(relDirLink1, B_OK, true);
	InitTest1DirPaths(relDirLink2, B_OK, true);
	InitTest1DirPaths(relDirLink3, B_OK, true);
	InitTest1DirPaths(relDirLink4, B_OK, true);
	InitTest1DirPaths(absFileLink1, B_OK, true);
	InitTest1DirPaths(absFileLink2, B_OK, true);
	InitTest1DirPaths(absFileLink3, B_OK, true);
	InitTest1DirPaths(absFileLink4, B_OK, true);
	InitTest1DirPaths(relFileLink1, B_OK, true);
	InitTest1DirPaths(relFileLink2, B_OK, true);
	InitTest1DirPaths(relFileLink3, B_OK, true);
	InitTest1DirPaths(relFileLink4, B_OK, true);
	InitTest1DirPaths(absCyclicLink1, B_LINK_LIMIT, true);
	InitTest1DirPaths(relCyclicLink1, B_LINK_LIMIT, true);
	InitTest1DirPaths(absBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest1DirPaths(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest1DirPaths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
// OBOS: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
	InitTest1DirPaths(tooLongDir16, B_OK, true);
#endif

	// special cases (root dir)
	NextSubTest();
	{
		BDirectory dir("/");
		BEntry entry(&dir, ".");
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// special cases (fs root dir)
	NextSubTest();
	{
		BDirectory dir("/");
		BEntry entry(&dir, "boot");
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// NULL path
	NextSubTest();
	{
		BDirectory dir("/");
		BEntry entry(&dir, (const char*)NULL);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// bad args (NULL dir)
// R5: crashs
#if !TEST_R5
	NextSubTest();
	{
		chdir("/");
		BEntry entry((const BDirectory*)NULL, "tmp");
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
		RestoreCWD();
	}
#endif
	// strange args (badly initialized dir, absolute path)
	NextSubTest();
	{
		BDirectory dir(badEntry1.cpath);
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
		BEntry entry(&dir, "/tmp");
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// bad args (NULL dir and path)
// R5: crashs
#if !TEST_R5
	NextSubTest();
	{
		BEntry entry((const BDirectory*)NULL, (const char*)NULL);
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
	}
#endif
	// bad args(NULL dir, absolute path)
// R5: crashs
#if !TEST_R5
	NextSubTest();
	{
		BEntry entry((const BDirectory*)NULL, "/tmp");
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
	}
#endif
}

// InitTest2Paths
void
EntryTest::InitTest2Paths(TestEntry &_testEntry, status_t error, bool traverse)
{
	TestEntry *testEntry = &_testEntry;
	BEntry entry;
	// absolute path
	NextSubTest();
	{
//printf("%s\n", testEntry->cpath);
		status_t result = entry.SetTo(testEntry->cpath, traverse);
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
		CPPUNIT_ASSERT( fuzzy_equals(result, error) );
		CPPUNIT_ASSERT( fuzzy_equals(entry.InitCheck(), error) );
		if (result == B_OK)
			examine_entry(entry, testEntry, traverse);
	}
	// relative path
	NextSubTest();
	{
//printf("%s\n", testEntry->cpath);
		if (chdir(testEntry->super->cpath) == 0) {
			status_t result = entry.SetTo(testEntry->cname, traverse);
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			CPPUNIT_ASSERT( fuzzy_equals(entry.InitCheck(), error) );
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
			RestoreCWD();
		}
	}
}

// InitTest2Refs
void
EntryTest::InitTest2Refs(TestEntry &_testEntry, status_t error, bool traverse)
{
	TestEntry *testEntry = &_testEntry;
	BEntry entry;
	// absolute path
	NextSubTest();
	{
//printf("%s\n", testEntry->cpath);
		status_t result = entry.SetTo(&testEntry->get_ref(), traverse);
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
		CPPUNIT_ASSERT( fuzzy_equals(result, error) );
		CPPUNIT_ASSERT( fuzzy_equals(entry.InitCheck(), error) );
		if (result == B_OK)
			examine_entry(entry, testEntry, traverse);
	}
}

// InitTest2DirPaths
void
EntryTest::InitTest2DirPaths(TestEntry &_testEntry, status_t error,
							 bool traverse)
{
	TestEntry *testEntry = &_testEntry;
	BEntry entry;
	// absolute path
	NextSubTest();
	{
		if (!testEntry->isBad()
			&& testEntry->path.length() < B_PATH_NAME_LENGTH) {
//printf("%s\n", testEntry->cpath);
			BDirectory dir("/boot/home/Desktop");
			CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
			status_t result = entry.SetTo(&dir, testEntry->cpath, traverse);
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			CPPUNIT_ASSERT( fuzzy_equals(entry.InitCheck(), error) );
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}
	// relative path (one level)
	NextSubTest();
	{
		if (!testEntry->isBad()
			&& testEntry->super->path.length() < B_PATH_NAME_LENGTH) {
//printf("%s + %s\n", testEntry->super->cpath, testEntry->cname);
			BDirectory dir(testEntry->super->cpath);
			CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
			status_t result = entry.SetTo(&dir, testEntry->cname, traverse);
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			CPPUNIT_ASSERT( fuzzy_equals(entry.InitCheck(), error) );
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}
	// relative path (two levels)
	NextSubTest();
	{
		if (!testEntry->super->isBad() && !testEntry->super->super->isBad()) {
			string entryName  = testEntry->super->name + "/" + testEntry->name;
//printf("%s + %s\n", testEntry->super->super->cpath, entryName.c_str());
			BDirectory dir(testEntry->super->super->cpath);
			CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
			status_t result = entry.SetTo(&dir, entryName.c_str(), traverse);
if (!fuzzy_equals(result, error))
printf("error: %lx (%lx)\n", result, error);
			CPPUNIT_ASSERT( fuzzy_equals(result, error) );
			CPPUNIT_ASSERT( fuzzy_equals(entry.InitCheck(), error) );
			if (result == B_OK)
				examine_entry(entry, testEntry, traverse);
		}
	}
}

// InitTest2
void
EntryTest::InitTest2()
{
	BEntry entry;
	// 2. SetTo(const char *, bool)
	// don't traverse
	InitTest2Paths(dir1, B_OK);
	InitTest2Paths(dir2, B_OK);
	InitTest2Paths(file1, B_OK);
	InitTest2Paths(subDir1, B_OK);
	InitTest2Paths(abstractEntry1, B_OK);
	InitTest2Paths(badEntry1, B_ENTRY_NOT_FOUND);
	InitTest2Paths(absDirLink1, B_OK);
	InitTest2Paths(absDirLink2, B_OK);
	InitTest2Paths(absDirLink3, B_OK);
	InitTest2Paths(absDirLink4, B_OK);
	InitTest2Paths(relDirLink1, B_OK);
	InitTest2Paths(relDirLink2, B_OK);
	InitTest2Paths(relDirLink3, B_OK);
	InitTest2Paths(relDirLink4, B_OK);
	InitTest2Paths(absFileLink1, B_OK);
	InitTest2Paths(absFileLink2, B_OK);
	InitTest2Paths(absFileLink3, B_OK);
	InitTest2Paths(absFileLink4, B_OK);
	InitTest2Paths(relFileLink1, B_OK);
	InitTest2Paths(relFileLink2, B_OK);
	InitTest2Paths(relFileLink3, B_OK);
	InitTest2Paths(relFileLink4, B_OK);
	InitTest2Paths(absCyclicLink1, B_OK);
	InitTest2Paths(relCyclicLink1, B_OK);
	InitTest2Paths(absBadLink1, B_OK);
	InitTest2Paths(absBadLink2, B_OK);
	InitTest2Paths(absBadLink3, B_OK);
	InitTest2Paths(absBadLink4, B_OK);
	InitTest2Paths(relBadLink1, B_OK);
	InitTest2Paths(relBadLink2, B_OK);
	InitTest2Paths(relBadLink3, B_OK);
	InitTest2Paths(relBadLink4, B_OK);
	InitTest2Paths(absVeryBadLink1, B_OK);
	InitTest2Paths(absVeryBadLink2, B_OK);
	InitTest2Paths(absVeryBadLink3, B_OK);
	InitTest2Paths(absVeryBadLink4, B_OK);
	InitTest2Paths(relVeryBadLink1, B_OK);
	InitTest2Paths(relVeryBadLink2, B_OK);
	InitTest2Paths(relVeryBadLink3, B_OK);
	InitTest2Paths(relVeryBadLink4, B_OK);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest2Paths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
// R5: returns B_ERROR instead of B_NAME_TOO_LONG
	InitTest2Paths(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG));
	// traverse
	InitTest2Paths(dir1, B_OK, true);
	InitTest2Paths(dir2, B_OK, true);
	InitTest2Paths(file1, B_OK, true);
	InitTest2Paths(subDir1, B_OK, true);
	InitTest2Paths(abstractEntry1, B_OK, true);
	InitTest2Paths(badEntry1, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absDirLink1, B_OK, true);
	InitTest2Paths(absDirLink2, B_OK, true);
	InitTest2Paths(absDirLink3, B_OK, true);
	InitTest2Paths(absDirLink4, B_OK, true);
	InitTest2Paths(relDirLink1, B_OK, true);
	InitTest2Paths(relDirLink2, B_OK, true);
	InitTest2Paths(relDirLink3, B_OK, true);
	InitTest2Paths(relDirLink4, B_OK, true);
	InitTest2Paths(absFileLink1, B_OK, true);
	InitTest2Paths(absFileLink2, B_OK, true);
	InitTest2Paths(absFileLink3, B_OK, true);
	InitTest2Paths(absFileLink4, B_OK, true);
	InitTest2Paths(relFileLink1, B_OK, true);
	InitTest2Paths(relFileLink2, B_OK, true);
	InitTest2Paths(relFileLink3, B_OK, true);
	InitTest2Paths(relFileLink4, B_OK, true);
	InitTest2Paths(absCyclicLink1, B_LINK_LIMIT, true);
	InitTest2Paths(relCyclicLink1, B_LINK_LIMIT, true);
	InitTest2Paths(absBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Paths(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest2Paths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
// R5: returns B_ERROR instead of B_NAME_TOO_LONG
	InitTest2Paths(tooLongDir16, fuzzy_error(B_ERROR, B_NAME_TOO_LONG), true);
	// special cases (root dir)
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	entry.Unset();
	// special cases (fs root dir)
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	entry.Unset();
	// bad args
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo((const char*)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
	entry.Unset();

	// 3. BEntry(const entry_ref *, bool)
	// don't traverse
	InitTest2Refs(dir1, B_OK);
	InitTest2Refs(dir2, B_OK);
	InitTest2Refs(file1, B_OK);
	InitTest2Refs(subDir1, B_OK);
	InitTest2Refs(abstractEntry1, B_OK);
	InitTest2Refs(absDirLink1, B_OK);
	InitTest2Refs(absDirLink2, B_OK);
	InitTest2Refs(absDirLink3, B_OK);
	InitTest2Refs(absDirLink4, B_OK);
	InitTest2Refs(relDirLink1, B_OK);
	InitTest2Refs(relDirLink2, B_OK);
	InitTest2Refs(relDirLink3, B_OK);
	InitTest2Refs(relDirLink4, B_OK);
	InitTest2Refs(absFileLink1, B_OK);
	InitTest2Refs(absFileLink2, B_OK);
	InitTest2Refs(absFileLink3, B_OK);
	InitTest2Refs(absFileLink4, B_OK);
	InitTest2Refs(relFileLink1, B_OK);
	InitTest2Refs(relFileLink2, B_OK);
	InitTest2Refs(relFileLink3, B_OK);
	InitTest2Refs(relFileLink4, B_OK);
	InitTest2Refs(absCyclicLink1, B_OK);
	InitTest2Refs(relCyclicLink1, B_OK);
	InitTest2Refs(absBadLink1, B_OK);
	InitTest2Refs(absBadLink2, B_OK);
	InitTest2Refs(absBadLink3, B_OK);
	InitTest2Refs(absBadLink4, B_OK);
	InitTest2Refs(relBadLink1, B_OK);
	InitTest2Refs(relBadLink2, B_OK);
	InitTest2Refs(relBadLink3, B_OK);
	InitTest2Refs(relBadLink4, B_OK);
	InitTest2Refs(absVeryBadLink1, B_OK);
	InitTest2Refs(absVeryBadLink2, B_OK);
	InitTest2Refs(absVeryBadLink3, B_OK);
	InitTest2Refs(absVeryBadLink4, B_OK);
	InitTest2Refs(relVeryBadLink1, B_OK);
	InitTest2Refs(relVeryBadLink2, B_OK);
	InitTest2Refs(relVeryBadLink3, B_OK);
	InitTest2Refs(relVeryBadLink4, B_OK);
	// traverse
	InitTest2Refs(dir1, B_OK, true);
	InitTest2Refs(dir2, B_OK, true);
	InitTest2Refs(file1, B_OK, true);
	InitTest2Refs(subDir1, B_OK, true);
	InitTest2Refs(abstractEntry1, B_OK, true);
	InitTest2Refs(absDirLink1, B_OK, true);
	InitTest2Refs(absDirLink2, B_OK, true);
	InitTest2Refs(absDirLink3, B_OK, true);
	InitTest2Refs(absDirLink4, B_OK, true);
	InitTest2Refs(relDirLink1, B_OK, true);
	InitTest2Refs(relDirLink2, B_OK, true);
	InitTest2Refs(relDirLink3, B_OK, true);
	InitTest2Refs(relDirLink4, B_OK, true);
	InitTest2Refs(absFileLink1, B_OK, true);
	InitTest2Refs(absFileLink2, B_OK, true);
	InitTest2Refs(absFileLink3, B_OK, true);
	InitTest2Refs(absFileLink4, B_OK, true);
	InitTest2Refs(relFileLink1, B_OK, true);
	InitTest2Refs(relFileLink2, B_OK, true);
	InitTest2Refs(relFileLink3, B_OK, true);
	InitTest2Refs(relFileLink4, B_OK, true);
	InitTest2Refs(absCyclicLink1, B_LINK_LIMIT, true);
	InitTest2Refs(relCyclicLink1, B_LINK_LIMIT, true);
	InitTest2Refs(absBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2Refs(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	// bad args
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo((const entry_ref*)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
	entry.Unset();

	// 4. BEntry(const BDirectory *, const char *, bool)
	// don't traverse
	InitTest2DirPaths(dir1, B_OK);
	InitTest2DirPaths(dir2, B_OK);
	InitTest2DirPaths(file1, B_OK);
	InitTest2DirPaths(subDir1, B_OK);
	InitTest2DirPaths(abstractEntry1, B_OK);
	InitTest2DirPaths(badEntry1, B_ENTRY_NOT_FOUND);
	InitTest2DirPaths(absDirLink1, B_OK);
	InitTest2DirPaths(absDirLink2, B_OK);
	InitTest2DirPaths(absDirLink3, B_OK);
	InitTest2DirPaths(absDirLink4, B_OK);
	InitTest2DirPaths(relDirLink1, B_OK);
	InitTest2DirPaths(relDirLink2, B_OK);
	InitTest2DirPaths(relDirLink3, B_OK);
	InitTest2DirPaths(relDirLink4, B_OK);
	InitTest2DirPaths(absFileLink1, B_OK);
	InitTest2DirPaths(absFileLink2, B_OK);
	InitTest2DirPaths(absFileLink3, B_OK);
	InitTest2DirPaths(absFileLink4, B_OK);
	InitTest2DirPaths(relFileLink1, B_OK);
	InitTest2DirPaths(relFileLink2, B_OK);
	InitTest2DirPaths(relFileLink3, B_OK);
	InitTest2DirPaths(relFileLink4, B_OK);
	InitTest2DirPaths(absCyclicLink1, B_OK);
	InitTest2DirPaths(relCyclicLink1, B_OK);
	InitTest2DirPaths(absBadLink1, B_OK);
	InitTest2DirPaths(absBadLink2, B_OK);
	InitTest2DirPaths(absBadLink3, B_OK);
	InitTest2DirPaths(absBadLink4, B_OK);
	InitTest2DirPaths(relBadLink1, B_OK);
	InitTest2DirPaths(relBadLink2, B_OK);
	InitTest2DirPaths(relBadLink3, B_OK);
	InitTest2DirPaths(relBadLink4, B_OK);
	InitTest2DirPaths(absVeryBadLink1, B_OK);
	InitTest2DirPaths(absVeryBadLink2, B_OK);
	InitTest2DirPaths(absVeryBadLink3, B_OK);
	InitTest2DirPaths(absVeryBadLink4, B_OK);
	InitTest2DirPaths(relVeryBadLink1, B_OK);
	InitTest2DirPaths(relVeryBadLink2, B_OK);
	InitTest2DirPaths(relVeryBadLink3, B_OK);
	InitTest2DirPaths(relVeryBadLink4, B_OK);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest2DirPaths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG));
// OBOS: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
	InitTest2DirPaths(tooLongDir16, B_OK, true);
#endif
	// traverse
	InitTest2DirPaths(dir1, B_OK, true);
	InitTest2DirPaths(dir2, B_OK, true);
	InitTest2DirPaths(file1, B_OK, true);
	InitTest2DirPaths(subDir1, B_OK, true);
	InitTest2DirPaths(abstractEntry1, B_OK, true);
	InitTest2DirPaths(badEntry1, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absDirLink1, B_OK, true);
	InitTest2DirPaths(absDirLink2, B_OK, true);
	InitTest2DirPaths(absDirLink3, B_OK, true);
	InitTest2DirPaths(absDirLink4, B_OK, true);
	InitTest2DirPaths(relDirLink1, B_OK, true);
	InitTest2DirPaths(relDirLink2, B_OK, true);
	InitTest2DirPaths(relDirLink3, B_OK, true);
	InitTest2DirPaths(relDirLink4, B_OK, true);
	InitTest2DirPaths(absFileLink1, B_OK, true);
	InitTest2DirPaths(absFileLink2, B_OK, true);
	InitTest2DirPaths(absFileLink3, B_OK, true);
	InitTest2DirPaths(absFileLink4, B_OK, true);
	InitTest2DirPaths(relFileLink1, B_OK, true);
	InitTest2DirPaths(relFileLink2, B_OK, true);
	InitTest2DirPaths(relFileLink3, B_OK, true);
	InitTest2DirPaths(relFileLink4, B_OK, true);
	InitTest2DirPaths(absCyclicLink1, B_LINK_LIMIT, true);
	InitTest2DirPaths(relCyclicLink1, B_LINK_LIMIT, true);
	InitTest2DirPaths(absBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(absVeryBadLink4, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relVeryBadLink1, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relVeryBadLink2, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relVeryBadLink3, B_ENTRY_NOT_FOUND, true);
	InitTest2DirPaths(relVeryBadLink4, B_ENTRY_NOT_FOUND, true);
// R5: returns E2BIG instead of B_NAME_TOO_LONG
	InitTest2DirPaths(tooLongEntry1, fuzzy_error(E2BIG, B_NAME_TOO_LONG), true);
// OBOS: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
	InitTest2DirPaths(tooLongDir16, B_OK, true);
#endif
	// special cases (root dir)
	NextSubTest();
	{
		BDirectory dir("/");
		CPPUNIT_ASSERT( entry.SetTo(&dir, ".") == B_OK );
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry.Unset();
	}
	// special cases (fs root dir)
	NextSubTest();
	{
		BDirectory dir("/");
		CPPUNIT_ASSERT( entry.SetTo(&dir, "boot") == B_OK );
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry.Unset();
	}
	// NULL path
	NextSubTest();
	{
		BDirectory dir("/");
		CPPUNIT_ASSERT( entry.SetTo(&dir, (const char*)NULL) == B_OK );
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry.Unset();
	}
	// bad args (NULL dir)
// R5: crashs
#if !TEST_R5
	NextSubTest();
	{
		chdir("/");
		CPPUNIT_ASSERT( entry.SetTo((const BDirectory*)NULL, "tmp")
						== B_BAD_VALUE );
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
		RestoreCWD();
		entry.Unset();
	}
#endif
	// strange args (badly initialized dir, absolute path)
	NextSubTest();
	{
		BDirectory dir(badEntry1.cpath);
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( entry.SetTo(&dir, "/tmp") == B_OK );
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	}
	// bad args (NULL dir and path)
// R5: crashs
#if !TEST_R5
	NextSubTest();
	{
		CPPUNIT_ASSERT( entry.SetTo((const BDirectory*)NULL, (const char*)NULL)
						== B_BAD_VALUE );
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
		entry.Unset();
	}
#endif
	// bad args(NULL dir, absolute path)
// R5: crashs
#if !TEST_R5
	NextSubTest();
	{
		CPPUNIT_ASSERT( entry.SetTo((const BDirectory*)NULL, "/tmp")
						== B_BAD_VALUE );
		CPPUNIT_ASSERT( entry.InitCheck() == B_BAD_VALUE );
		entry.Unset();
	}
#endif
}

// SpecialGetCasesTest
//
// Tests special (mostly error) cases for Exists(), GetPath(), GetName(),
// GetParent() and GetRef(). The other cases have already been tested in
// InitTest1/2().
void
EntryTest::SpecialGetCasesTest()
{
	BEntry entry;
	// 1. Exists()
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.Exists() == false );
	entry.Unset();	
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.Exists() == false );
	entry.Unset();	
	// root
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( entry.Exists() == true );
	entry.Unset();	

	// 2. GetPath()
	BPath path;
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_NO_INIT );
	entry.Unset();	
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_NO_INIT );
	entry.Unset();	
	// too long pathname
// OBOS: Fails, because the implementation concatenates the dir and leaf
// 		 name.
#if !TEST_OBOS /* !!!POSIX ONLY!!! */
	NextSubTest();
	BDirectory dir(tooLongDir16.super->super->cpath);
	string entryName = tooLongDir16.super->name + "/" + tooLongDir16.name;
	CPPUNIT_ASSERT( entry.SetTo(&dir, entryName.c_str()) == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == tooLongDir16.cpath );
	entry.Unset();	
#endif

	// 3. GetName()
	char name[B_FILE_NAME_LENGTH + 1];
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.GetName(name) == B_NO_INIT );
	entry.Unset();	
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.GetName(name) == B_NO_INIT );
	entry.Unset();	

	// 4. GetParent(BEntry *)
	BEntry parentEntry;
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.GetParent(&parentEntry) == B_NO_INIT );
	entry.Unset();	
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.GetParent(&parentEntry) == B_NO_INIT );
	entry.Unset();	
	// parent of root dir
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( entry.GetParent(&parentEntry) == B_ENTRY_NOT_FOUND );
	entry.Unset();	

	// 5. GetParent(BDirectory *)
	BDirectory parentDir;
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.GetParent(&parentDir) == B_NO_INIT );
	entry.Unset();	
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.GetParent(&parentDir) == B_NO_INIT );
	entry.Unset();	
	// parent of root dir
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( entry.GetParent(&parentDir) == B_ENTRY_NOT_FOUND );
	entry.Unset();	

	// 6. GetRef()
	entry_ref ref, ref2;
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_NO_INIT );
	entry.Unset();	
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_NO_INIT );
	entry.Unset();	
	// ref for root dir
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	entry.Unset();	
}

// RenameTestEntry
void
EntryTest::RenameTestEntry(TestEntry *testEntry, TestEntry *newTestEntry,
						   string newName, bool existing, bool clobber,
						   status_t error, uint32 kind)
{
	NextSubTest();
	BEntry entry;
	BDirectory dir;
	// get all the names
	string pathname = testEntry->path; 
	string dirname = testEntry->super->path;
	string newPathname = newTestEntry->path;
//printf("path: `%s', dir: `%s', new name: `%s'\n", pathname.c_str(),
//dirname.c_str(), newPathname.c_str());
	// create the entries
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
	CPPUNIT_ASSERT( entry.SetTo(pathname.c_str()) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(dirname.c_str()) == B_OK );
	status_t result = entry.Rename(newName.c_str(), clobber);
if (result != error) {
printf("`%s'.Rename(`%s', %d): ", pathname.c_str(), newName.c_str(), clobber);
printf("error: %lx (%lx)\n", result, error);
}
	CPPUNIT_ASSERT( result == error );
	// check and cleanup
	if (error == B_OK) {
		switch (kind) {
			case B_FILE_NODE:
				CPPUNIT_ASSERT( !PingFile(pathname.c_str()) );
				CPPUNIT_ASSERT( PingFile(newPathname.c_str()) );
				break;
			case B_DIRECTORY_NODE:
				CPPUNIT_ASSERT( !PingDir(pathname.c_str()) );
				CPPUNIT_ASSERT( PingDir(newPathname.c_str()) );
				break;
			case B_SYMLINK_NODE:
				CPPUNIT_ASSERT( !PingLink(pathname.c_str()) );
				CPPUNIT_ASSERT( PingLink(newPathname.c_str(), file1.cpath) );
				break;
		}
		RemoveFile(newPathname.c_str());
	} else {
		switch (kind) {
			case B_FILE_NODE:
				CPPUNIT_ASSERT( PingFile(pathname.c_str()) );
				break;
			case B_DIRECTORY_NODE:
				CPPUNIT_ASSERT( PingDir(pathname.c_str()) );
				break;
			case B_SYMLINK_NODE:
				CPPUNIT_ASSERT( PingLink(pathname.c_str(), file1.cpath) );
				break;
		}
		if (existing) {
			CPPUNIT_ASSERT( PingFile(newPathname.c_str()) );
			RemoveFile(newPathname.c_str());
		}
		RemoveFile(pathname.c_str());
	}
}

// RenameTestEntry
void
EntryTest::RenameTestEntry(TestEntry *testEntry, TestEntry *newTestEntry,
						   bool existing, bool clobber, status_t error,
						   uint32 kind)
{
	// relative path
	string relPath = get_shortest_relative_path(testEntry->super,
												newTestEntry);
	if (relPath.length() > 0) {
		RenameTestEntry(testEntry, newTestEntry, relPath, existing,
						clobber, error, B_FILE_NODE);
	}
	// absolute path
	RenameTestEntry(testEntry, newTestEntry, newTestEntry->path, existing,
					clobber, error, B_FILE_NODE);
}

// RenameTestFile
void
EntryTest::RenameTestFile(TestEntry *testEntry, TestEntry *newTestEntry,
						  bool existing, bool clobber, status_t error)
{
	RenameTestEntry(testEntry, newTestEntry, existing, clobber, error,
					B_FILE_NODE);
}

// RenameTestDir
void
EntryTest::RenameTestDir(TestEntry *testEntry, TestEntry *newTestEntry,
						 bool existing, bool clobber, status_t error)
{
	RenameTestEntry(testEntry, newTestEntry, existing, clobber, error,
					B_DIRECTORY_NODE);
}

// RenameTestLink
void
EntryTest::RenameTestLink(TestEntry *testEntry, TestEntry *newTestEntry,
						  bool existing, bool clobber, status_t error)
{
	RenameTestEntry(testEntry, newTestEntry, existing, clobber, error,
					B_SYMLINK_NODE);
}

// RenameTest
void
EntryTest::RenameTest()
{
	BDirectory dir;
	BEntry entry;
	// file
	// same dir
	RenameTestFile(&file2, &file2, false, false, B_FILE_EXISTS);
	RenameTestFile(&file2, &file2, false, true, B_NOT_ALLOWED);
	RenameTestFile(&file2, &file4, false, false, B_OK);
	// different dir
	RenameTestFile(&file2, &file3, false, false, B_OK);
	// different dir, existing file, clobber
	RenameTestFile(&file2, &file3, true, true, B_OK);
	// different dir, existing file, no clobber
	RenameTestFile(&file2, &file3, true, false, B_FILE_EXISTS);
	// dir
	// same dir
	RenameTestDir(&file2, &file2, false, false, B_FILE_EXISTS);
	RenameTestDir(&file2, &file2, false, true, B_NOT_ALLOWED);
	RenameTestDir(&file2, &file4, false, false, B_OK);
	// different dir
	RenameTestDir(&file2, &file3, false, false, B_OK);
	// different dir, existing file, clobber
	RenameTestDir(&file2, &file3, true, true, B_OK);
	// different dir, existing file, no clobber
	RenameTestDir(&file2, &file3, true, false, B_FILE_EXISTS);
	// link
	// same dir
	RenameTestLink(&file2, &file2, false, false, B_FILE_EXISTS);
	RenameTestLink(&file2, &file2, false, true, B_NOT_ALLOWED);
	RenameTestLink(&file2, &file4, false, false, B_OK);
	// different dir
	RenameTestLink(&file2, &file3, false, false, B_OK);
	// different dir, existing file, clobber
	RenameTestLink(&file2, &file3, true, true, B_OK);
	// different dir, existing file, no clobber
	RenameTestLink(&file2, &file3, true, false, B_FILE_EXISTS);

	// try to clobber a non-empty directory
	NextSubTest();
	CreateFile(file3.cpath);
	CPPUNIT_ASSERT( entry.SetTo(file3.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Rename(dir1.cpath, true) == B_DIRECTORY_NOT_EMPTY );
	CPPUNIT_ASSERT( PingDir(dir1.cpath) );
	CPPUNIT_ASSERT( PingFile(file3.cpath, &entry) );
	RemoveFile(file3.cpath);
	entry.Unset();
	dir.Unset();
	// clobber an empty directory
	NextSubTest();
	CreateFile(file3.cpath);
	CPPUNIT_ASSERT( entry.SetTo(file3.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Rename(subDir1.cpath, true) == B_OK );
	CPPUNIT_ASSERT( PingFile(subDir1.cpath, &entry) );
	CPPUNIT_ASSERT( !PingFile(file3.cpath) );
	RemoveFile(subDir1.cpath);
	entry.Unset();
	dir.Unset();
	// abstract entry
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(file2.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Rename(file4.cname) == B_ENTRY_NOT_FOUND );
	entry.Unset();
	dir.Unset();
	// uninitialized entry
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.Rename(file4.cpath) == B_NO_INIT );
	entry.Unset();
	dir.Unset();
	// badly initialized entry
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.Rename(file4.cpath) == B_NO_INIT );
	entry.Unset();
	dir.Unset();
	// Verify attempts to rename root
	NextSubTest();
	BEntry root("/");
	CPPUNIT_ASSERT( root.Rename("/", false) == B_FILE_EXISTS );
	CPPUNIT_ASSERT( root.Rename("/", true) == B_NOT_ALLOWED );
	// Verify abstract entries
	NextSubTest();
	BEntry abstract(abstractEntry1.cpath);
	CPPUNIT_ASSERT( abstract.InitCheck() == B_OK );
	CPPUNIT_ASSERT( !abstract.Exists() );
	CPPUNIT_ASSERT( abstract.Rename("/boot/DoesntMatter") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( abstract.Rename("/boot/DontMatter", true) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( abstract.Rename("/DoesntMatter") == B_CROSS_DEVICE_LINK );
	CPPUNIT_ASSERT( abstract.Rename("/DontMatter", true) == B_CROSS_DEVICE_LINK );
	// bad args
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(file1.cpath) == B_OK );
	CPPUNIT_ASSERT( equals(entry.Rename(NULL, false), B_FILE_EXISTS,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(entry.Rename(NULL, true), B_NOT_ALLOWED,
						   B_BAD_VALUE) );
}

// MoveToTestEntry
void
EntryTest::MoveToTestEntry(TestEntry *testEntry, TestEntry *testDir,
						   string newName, bool existing, bool clobber,
						   status_t error, uint32 kind)
{
	NextSubTest();
	BEntry entry;
	BDirectory dir;
	// get all the names
	string pathname = testEntry->path; 
	string dirname = testDir->path;
	string newPathname = dirname + "/";
	if (newName.length() == 0)
		newPathname += testEntry->name;
	else {
		// check, if the new path is absolute
		if (newName.find("/") == 0)
			newPathname = newName;
		else
			newPathname += newName;
	}
//printf("path: `%s', dir: `%s', new name: `%s'\n", pathname.c_str(),
//dirname.c_str(), newPathname.c_str());
	// create the entries
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
	CPPUNIT_ASSERT( entry.SetTo(pathname.c_str()) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(dirname.c_str()) == B_OK );
	if (newName.length() == 0) {
		status_t result = entry.MoveTo(&dir, NULL, clobber);
if (result != error) {
printf("`%s'.MoveTo(`%s', NULL, %d): ", pathname.c_str(), dirname.c_str(), clobber);
printf("error: %lx (%lx)\n", result, error);
}
		CPPUNIT_ASSERT( result == error );
	} else {
		status_t result = entry.MoveTo(&dir, newName.c_str(), clobber);
if (result != error) {
printf("`%s'.MoveTo(`%s', `%s', %d): ", pathname.c_str(), newName.c_str(), dirname.c_str(), clobber);
printf("error: %lx (%lx)\n", result, error);
}
		CPPUNIT_ASSERT( result == error );
	}
	// check and cleanup
	if (error == B_OK) {
		switch (kind) {
			case B_FILE_NODE:
				CPPUNIT_ASSERT( !PingFile(pathname.c_str()) );
				CPPUNIT_ASSERT( PingFile(newPathname.c_str()) );
				break;
			case B_DIRECTORY_NODE:
				CPPUNIT_ASSERT( !PingDir(pathname.c_str()) );
				CPPUNIT_ASSERT( PingDir(newPathname.c_str()) );
				break;
			case B_SYMLINK_NODE:
				CPPUNIT_ASSERT( !PingLink(pathname.c_str()) );
				CPPUNIT_ASSERT( PingLink(newPathname.c_str(), file1.cpath) );
				break;
		}
		RemoveFile(newPathname.c_str());
	} else {
		switch (kind) {
			case B_FILE_NODE:
				CPPUNIT_ASSERT( PingFile(pathname.c_str()) );
				break;
			case B_DIRECTORY_NODE:
				CPPUNIT_ASSERT( PingDir(pathname.c_str()) );
				break;
			case B_SYMLINK_NODE:
				CPPUNIT_ASSERT( PingLink(pathname.c_str(), file1.cpath) );
				break;
		}
		if (existing) {
			CPPUNIT_ASSERT( PingFile(newPathname.c_str()) );
			RemoveFile(newPathname.c_str());
		}
		RemoveFile(pathname.c_str());
	}
}

// MoveToTestEntry
void
EntryTest::MoveToTestEntry(TestEntry *testEntry, TestEntry *testDir,
						   TestEntry *newTestEntry, bool existing,
						   bool clobber, status_t error, uint32 kind)
{
	if (newTestEntry) {
		// Here is the right place to play a little bit with the dir and path
		// arguments. At this time we only pass the leaf name and the
		// absolute path name.
		MoveToTestEntry(testEntry, testDir, newTestEntry->name, existing,
						clobber, error, B_FILE_NODE);
		MoveToTestEntry(testEntry, &subDir1, newTestEntry->path, existing,
						clobber, error, B_FILE_NODE);
	} else {
		MoveToTestEntry(testEntry, testDir, "", existing, clobber, error,
						B_FILE_NODE);
	}
}

// MoveToTestFile
void
EntryTest::MoveToTestFile(TestEntry *testEntry, TestEntry *testDir,
						  TestEntry *newTestEntry, bool existing, bool clobber,
						  status_t error)
{
	MoveToTestEntry(testEntry, testDir, newTestEntry, existing, clobber, error,
					B_FILE_NODE);
}

// MoveToTestDir
void
EntryTest::MoveToTestDir(TestEntry *testEntry, TestEntry *testDir,
						 TestEntry *newTestEntry, bool existing, bool clobber,
						 status_t error)
{
	MoveToTestEntry(testEntry, testDir, newTestEntry, existing, clobber, error,
					B_DIRECTORY_NODE);
}

// MoveToTestLink
void
EntryTest::MoveToTestLink(TestEntry *testEntry, TestEntry *testDir,
						  TestEntry *newTestEntry, bool existing, bool clobber,
						  status_t error)
{
	MoveToTestEntry(testEntry, testDir, newTestEntry, existing, clobber, error,
					B_SYMLINK_NODE);
}

// MoveToTest
void
EntryTest::MoveToTest()
{
	BDirectory dir;
	BEntry entry;
	// 1. NULL path
	// file
	// same dir
	MoveToTestFile(&file2, file2.super, NULL, false, false, B_FILE_EXISTS);
	MoveToTestFile(&file2, file2.super, NULL, false, true, B_NOT_ALLOWED);
	// different dir
	MoveToTestFile(&file2, &dir2, NULL, false, false, B_OK);
	// different dir, existing file, clobber
	MoveToTestFile(&file2, &dir2, NULL, true, true, B_OK);
	// different dir, existing file, no clobber
	MoveToTestFile(&file2, &dir2, NULL, true, false, B_FILE_EXISTS);
	// dir
	// same dir
	MoveToTestDir(&file2, file2.super, NULL, false, false, B_FILE_EXISTS);
	MoveToTestDir(&file2, file2.super, NULL, false, true, B_NOT_ALLOWED);
	// different dir
	MoveToTestDir(&file2, &dir2, NULL, false, false, B_OK);
	// different dir, existing file, clobber
	MoveToTestDir(&file2, &dir2, NULL, true, true, B_OK);
	// different dir, existing file, no clobber
	MoveToTestDir(&file2, &dir2, NULL, true, false, B_FILE_EXISTS);
	// link
	// same dir
	MoveToTestLink(&file2, file2.super, NULL, false, false, B_FILE_EXISTS);
	MoveToTestLink(&file2, file2.super, NULL, false, true, B_NOT_ALLOWED);
	// different dir
	MoveToTestLink(&file2, &dir2, NULL, false, false, B_OK);
	// different dir, existing file, clobber
	MoveToTestLink(&file2, &dir2, NULL, true, true, B_OK);
	// different dir, existing file, no clobber
	MoveToTestLink(&file2, &dir2, NULL, true, false, B_FILE_EXISTS);

	// 2. non NULL path
	// file
	// same dir
	MoveToTestFile(&file2, file2.super, &file2, false, false,
				   B_FILE_EXISTS);
	MoveToTestFile(&file2, file2.super, &file2, false, true,
				   B_NOT_ALLOWED);
	MoveToTestFile(&file2, file2.super, &file3, false, false, B_OK);
	// different dir
	MoveToTestFile(&file2, &dir2, &file3, false, false, B_OK);
	// different dir, existing file, clobber
	MoveToTestFile(&file2, &dir2, &file3, true, true, B_OK);
	// different dir, existing file, no clobber
	MoveToTestFile(&file2, &dir2, &file3, true, false, B_FILE_EXISTS);
	// dir
	// same dir
	MoveToTestDir(&file2, file2.super, &file2, false, false,
				  B_FILE_EXISTS);
	MoveToTestDir(&file2, file2.super, &file2, false, true,
				  B_NOT_ALLOWED);
	MoveToTestDir(&file2, file2.super, &file3, false, false, B_OK);
	// different dir
	MoveToTestDir(&file2, &dir2, &file3, false, false, B_OK);
	// different dir, existing file, clobber
	MoveToTestDir(&file2, &dir2, &file3, true, true, B_OK);
	// different dir, existing file, no clobber
	MoveToTestDir(&file2, &dir2, &file3, true, false, B_FILE_EXISTS);
	// link
	// same dir
	MoveToTestLink(&file2, file2.super, &file2, false, false,
				   B_FILE_EXISTS);
	MoveToTestLink(&file2, file2.super, &file2, false, true,
				   B_NOT_ALLOWED);
	MoveToTestLink(&file2, file2.super, &file3, false, false, B_OK);
	// different dir
	MoveToTestLink(&file2, &dir2, &file3, false, false, B_OK);
	// different dir, existing file, clobber
	MoveToTestLink(&file2, &dir2, &file3, true, true, B_OK);
	// different dir, existing file, no clobber
	MoveToTestLink(&file2, &dir2, &file3, true, false, B_FILE_EXISTS);

	// try to clobber a non-empty directory
	CreateFile(file3.cpath);
	CPPUNIT_ASSERT( entry.SetTo(file3.cpath) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(dir1.super->cpath) == B_OK );
	CPPUNIT_ASSERT( entry.MoveTo(&dir, dir1.cname, true)
					== B_DIRECTORY_NOT_EMPTY );
	CPPUNIT_ASSERT( PingDir(dir1.cpath) );
	CPPUNIT_ASSERT( PingFile(file3.cpath, &entry) );
	RemoveFile(file3.cpath);
	entry.Unset();
	dir.Unset();
	// clobber an empty directory
	CreateFile(file3.cpath);
	CPPUNIT_ASSERT( entry.SetTo(file3.cpath) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(subDir1.super->cpath) == B_OK );
	CPPUNIT_ASSERT( entry.MoveTo(&dir, subDir1.cname, true) == B_OK );
	CPPUNIT_ASSERT( PingFile(subDir1.cpath, &entry) );
	CPPUNIT_ASSERT( !PingFile(file3.cpath) );
	RemoveFile(subDir1.cpath);
	entry.Unset();
	dir.Unset();
	// abstract entry
	CPPUNIT_ASSERT( entry.SetTo(file2.cpath) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(dir2.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.MoveTo(&dir) == B_ENTRY_NOT_FOUND );
	entry.Unset();
	dir.Unset();
	// uninitialized entry
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.SetTo(dir2.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.MoveTo(&dir) == B_NO_INIT );
	entry.Unset();
	dir.Unset();
	// badly initialized entry
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.SetTo(dir2.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.MoveTo(&dir) == B_NO_INIT );
	entry.Unset();
	dir.Unset();
	// bad args (NULL dir)
// R5: crashs
#if !TEST_R5
	CreateFile(file2.cpath);
	CPPUNIT_ASSERT( entry.SetTo(file2.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.MoveTo(NULL, file3.cpath) == B_BAD_VALUE );
	RemoveFile(file3.cpath);
	entry.Unset();
	dir.Unset();
#endif
	// uninitialized dir, absolute path
	CreateFile(file2.cpath);
	CPPUNIT_ASSERT( entry.SetTo(file2.cpath) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.MoveTo(&dir, file3.cpath) == B_BAD_VALUE );
	RemoveFile(file3.cpath);
	entry.Unset();
	dir.Unset();
}

// RemoveTest
void
EntryTest::RemoveTest()
{
	BEntry entry;
	// file
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(file1.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Remove() == B_OK );
	CPPUNIT_ASSERT( !PingFile(file1.cpath) );
	entry.Unset();
	// symlink
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(absFileLink1.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Remove() == B_OK );
	CPPUNIT_ASSERT( !PingLink(absFileLink1.cpath) );
	entry.Unset();
	// empty dir
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(subDir1.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Remove() == B_OK );
	CPPUNIT_ASSERT( !PingDir(subDir1.cpath) );
	entry.Unset();
	// non-empty dir
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(dir1.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Remove() == B_DIRECTORY_NOT_EMPTY );
	CPPUNIT_ASSERT( PingDir(dir1.cpath) );
	entry.Unset();
	// abstract entry
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(abstractEntry1.cpath) == B_OK );
	CPPUNIT_ASSERT( entry.Remove() == B_ENTRY_NOT_FOUND );
	entry.Unset();
	// uninitialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.Remove() == B_NO_INIT );
	entry.Unset();
	// badly initialized
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(badEntry1.cpath) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.Remove() == B_NO_INIT );
	entry.Unset();
}

// compareEntries
static
void
compareEntries(const BEntry &entry, const BEntry &entry2,
			   const TestEntry *testEntry, const TestEntry *testEntry2,
			   bool traversed, bool traversed2)
{
	if (!testEntry->isBad() && !testEntry2->isBad()
		&& (!traversed && testEntry->isConcrete())
		&& (!traversed2 && testEntry2->isConcrete())) {
//printf("compare: `%s', `%s'\n", testEntry->cpath, testEntry2->cpath);
//printf("InitCheck(): %x, %x\n", entry.InitCheck(), entry2.InitCheck());
		CPPUNIT_ASSERT( (entry == entry2) == (testEntry == testEntry2) );
		CPPUNIT_ASSERT( (entry == entry2) == (testEntry2 == testEntry) );
		CPPUNIT_ASSERT( (entry != entry2) == (testEntry != testEntry2) );
		CPPUNIT_ASSERT( (entry != entry2) == (testEntry2 != testEntry) );
	}
}

// ComparisonTest
void
EntryTest::ComparisonTest()
{
	// uninitialized
	NextSubTest();
	{
		BEntry entry;
		BEntry entry2;
		CPPUNIT_ASSERT( entry == entry2 );
		CPPUNIT_ASSERT( entry2 == entry );
		CPPUNIT_ASSERT( !(entry != entry2) );
		CPPUNIT_ASSERT( !(entry2 != entry) );
	}
	// initialized + uninitialized
	NextSubTest();
	{
		BEntry entry(file1.cpath);
		BEntry entry2;
		CPPUNIT_ASSERT( !(entry == entry2) );
		CPPUNIT_ASSERT( !(entry2 == entry) );
		CPPUNIT_ASSERT( entry != entry2 );
		CPPUNIT_ASSERT( entry2 != entry );
	}
	{
		BEntry entry;
		BEntry entry2(file1.cpath);
		CPPUNIT_ASSERT( !(entry == entry2) );
		CPPUNIT_ASSERT( !(entry2 == entry) );
		CPPUNIT_ASSERT( entry != entry2 );
		CPPUNIT_ASSERT( entry2 != entry );
	}
	// initialized
	TestEntry *testEntries[] = {
		&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
		&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4,
		&relDirLink1, &relDirLink2, &relDirLink3, &relDirLink4,
		&absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
		&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4,
		&absBadLink1, &absBadLink2, &absBadLink3, &absBadLink4,
		&relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
		&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
		&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4
	};
	int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
	for (int32 i = 0; i < testEntryCount; i++) {
		NextSubTest();
		TestEntry *testEntry = testEntries[i];
		TestEntry *traversedTestEntry = resolve_link(testEntry);
		BEntry entry(testEntry->cpath);
		BEntry traversedEntry(testEntry->cpath, true);
		for (int32 k = 0; k < testEntryCount; k++) {
			TestEntry *testEntry2 = testEntries[k];
			TestEntry *traversedTestEntry2 = resolve_link(testEntry2);
			BEntry entry2(testEntry2->cpath);
			BEntry traversedEntry2(testEntry2->cpath, true);
			compareEntries(entry, entry2, testEntry, testEntry2, false, false);
			compareEntries(traversedEntry, entry2,
						   traversedTestEntry, testEntry2, true, false);
			compareEntries(entry, traversedEntry2,
						   testEntry, traversedTestEntry2, false, true);
			compareEntries(traversedEntry, traversedEntry2,
						   traversedTestEntry, traversedTestEntry2,
						   true, true);
		}
	}
}

// AssignmentTest
void
EntryTest::AssignmentTest()
{
	// 1. copy constructor
	// uninitialized
	NextSubTest();
	{
		BEntry entry;
		CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
		BEntry entry2(entry);
// R5: returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(entry2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
		CPPUNIT_ASSERT( entry == entry2 );
	}
	// initialized
	TestEntry *testEntries[] = {
		&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
		&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4,
		&relDirLink1, &relDirLink2, &relDirLink3, &relDirLink4,
		&absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
		&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4,
		&absBadLink1, &absBadLink2, &absBadLink3, &absBadLink4,
		&relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
		&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
		&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4
	};
	int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
	for (int32 i = 0; i < testEntryCount; i++) {
		NextSubTest();
		TestEntry *testEntry = testEntries[i];
		BEntry entry(testEntry->cpath);
		BEntry entry2(entry);
		CPPUNIT_ASSERT( entry == entry2 );
		CPPUNIT_ASSERT( entry2 == entry );
		CPPUNIT_ASSERT( !(entry != entry2) );
		CPPUNIT_ASSERT( !(entry2 != entry) );
	}

	// 2. assignment operator
	// uninitialized
	NextSubTest();
	{
		BEntry entry;
		CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
		BEntry entry2;
		entry2 = entry;
// R5: returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(entry2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
		CPPUNIT_ASSERT( entry == entry2 );
	}
	NextSubTest();
	{
		BEntry entry;
		CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
		BEntry entry2(file1.cpath);
		entry2 = entry;
// R5: returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(entry2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
		CPPUNIT_ASSERT( entry == entry2 );
	}
	// initialized
	for (int32 i = 0; i < testEntryCount; i++) {
		NextSubTest();
		TestEntry *testEntry = testEntries[i];
		BEntry entry(testEntry->cpath);
		BEntry entry2;
		BEntry entry3(file1.cpath);
		entry2 = entry;
		entry3 = entry;
		CPPUNIT_ASSERT( entry == entry2 );
		CPPUNIT_ASSERT( entry2 == entry );
		CPPUNIT_ASSERT( !(entry != entry2) );
		CPPUNIT_ASSERT( !(entry2 != entry) );
		CPPUNIT_ASSERT( entry == entry3 );
		CPPUNIT_ASSERT( entry3 == entry );
		CPPUNIT_ASSERT( !(entry != entry3) );
		CPPUNIT_ASSERT( !(entry3 != entry) );
	}
}

// get_entry_ref_for_entry
static
status_t
get_entry_ref_for_entry(const char *dir, const char *leaf, entry_ref *ref)
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

// entry_ref >
bool
operator>(const entry_ref & a, const entry_ref & b)
{
	return (a.device > b.device
		|| (a.device == b.device
			&& (a.directory > b.directory
			|| (a.directory == b.directory
				&& (a.name != NULL && b.name == NULL
				|| (a.name != NULL && b.name != NULL
					&& strcmp(a.name, b.name) > 0))))));
}

// CFunctionsTest
void
EntryTest::CFunctionsTest()
{
	// get_ref_for_path(), <
	TestEntry *testEntries[] = {
		&dir1, &dir2, &file1, &subDir1, &abstractEntry1, &badEntry1,
		&absDirLink1, &absDirLink2, &absDirLink3, &absDirLink4,
		&relDirLink1, &relDirLink2, &relDirLink3, &relDirLink4,
		&absFileLink1, &absFileLink2, &absFileLink3, &absFileLink4,
		&relFileLink1, &relFileLink2, &relFileLink3, &relFileLink4,
		&absBadLink1, &absBadLink2, &absBadLink3, &absBadLink4,
		&relBadLink1, &relBadLink2, &relBadLink3, &relBadLink4,
		&absVeryBadLink1, &absVeryBadLink2, &absVeryBadLink3, &absVeryBadLink4,
		&relVeryBadLink1, &relVeryBadLink2, &relVeryBadLink3, &relVeryBadLink4
	};
	int32 testEntryCount = sizeof(testEntries) / sizeof(TestEntry*);
	for (int32 i = 0; i < testEntryCount; i++) {
		NextSubTest();
		TestEntry *testEntry = testEntries[i];
		const char *path = testEntry->cpath;
		entry_ref ref;
		if (testEntry->isBad())
			CPPUNIT_ASSERT( get_ref_for_path(path, &ref) == B_ENTRY_NOT_FOUND );
		else {
			CPPUNIT_ASSERT( get_ref_for_path(path, &ref) == B_OK );
			const entry_ref &testEntryRef = testEntry->get_ref();
			CPPUNIT_ASSERT( testEntryRef.device == ref.device );
			CPPUNIT_ASSERT( testEntryRef.directory == ref.directory );
			CPPUNIT_ASSERT( strcmp(testEntryRef.name, ref.name) == 0 );
			CPPUNIT_ASSERT( testEntryRef == ref );
			CPPUNIT_ASSERT( !(testEntryRef != ref) );
			CPPUNIT_ASSERT(  ref == testEntryRef );
			CPPUNIT_ASSERT(  !(ref != testEntryRef) );
			for (int32 k = 0; k < testEntryCount; k++) {
				TestEntry *testEntry2 = testEntries[k];
				const char *path2 = testEntry2->cpath;
				entry_ref ref2;
				if (!testEntry2->isBad()) {
					CPPUNIT_ASSERT( get_ref_for_path(path2, &ref2) == B_OK );
					int cmp = 0;
					if (ref > ref2)
						cmp = 1;
					else if (ref2 > ref)
						cmp = -1;
					CPPUNIT_ASSERT(  (ref == ref2) == (cmp == 0) );
					CPPUNIT_ASSERT(  (ref2 == ref) == (cmp == 0) );
					CPPUNIT_ASSERT(  (ref != ref2) == (cmp != 0) );
					CPPUNIT_ASSERT(  (ref2 != ref) == (cmp != 0) );
					CPPUNIT_ASSERT(  (ref < ref2) == (cmp < 0) );
					CPPUNIT_ASSERT(  (ref2 < ref) == (cmp > 0) );
				}
			}
		}
	}
	// root dir
	NextSubTest();
	entry_ref ref, ref2;
	CPPUNIT_ASSERT( get_ref_for_path("/", &ref) == B_OK );
	CPPUNIT_ASSERT( get_entry_ref_for_entry("/", ".", &ref2) == B_OK );
	CPPUNIT_ASSERT( ref.device == ref2.device );
	CPPUNIT_ASSERT( ref.directory == ref2.directory );
	CPPUNIT_ASSERT( strcmp(ref.name, ref2.name) == 0 );
	CPPUNIT_ASSERT(  ref == ref2 );
	// fs root dir
	NextSubTest();
	CPPUNIT_ASSERT( get_ref_for_path("/boot", &ref) == B_OK );
	CPPUNIT_ASSERT( get_entry_ref_for_entry("/", "boot", &ref2) == B_OK );
	CPPUNIT_ASSERT( ref.device == ref2.device );
	CPPUNIT_ASSERT( ref.directory == ref2.directory );
	CPPUNIT_ASSERT( strcmp(ref.name, ref2.name) == 0 );
	CPPUNIT_ASSERT(  ref == ref2 );
	// uninitialized
	NextSubTest();
	ref = entry_ref();
	ref2 = entry_ref();
	CPPUNIT_ASSERT(  ref == ref2 );
	CPPUNIT_ASSERT(  !(ref != ref2) );
	CPPUNIT_ASSERT(  !(ref < ref2) );
	CPPUNIT_ASSERT(  !(ref2 < ref) );
	CPPUNIT_ASSERT( get_entry_ref_for_entry("/", ".", &ref2) == B_OK );
	CPPUNIT_ASSERT(  !(ref == ref2) );
	CPPUNIT_ASSERT(  ref != ref2 );
	CPPUNIT_ASSERT(  ref < ref2 );
	CPPUNIT_ASSERT(  !(ref2 < ref) );
}


// isHarmlessPathname
bool
isHarmlessPathname(const char *path)
{
	bool result = false;
	if (path) {
		const char *harmlessDir = "/tmp/";
		result = (string(path).find(harmlessDir) == 0);
	}
if (!result)
printf("WARNING: `%s' is not a harmless pathname.\n", path);
	return result;
}

// CreateLink
void
EntryTest::CreateLink(const char *link, const char *target)
{
	if (link && target && isHarmlessPathname(link)) {
		execCommand(string("rm -rf ") + link
					+ " ; ln -s " + target + " " + link);
	}
}
	
// CreateFile
void
EntryTest::CreateFile(const char *file)
{
	if (file && isHarmlessPathname(file))
		execCommand(string("rm -rf ") + file + " ; touch " + file);
}
			
// CreateDir
void
EntryTest::CreateDir(const char *dir)
{
	if (dir  && isHarmlessPathname(dir))
		execCommand(string("rm -rf ") + dir + " ; mkdir " + dir);
}
			
// RemoveFile
void
EntryTest::RemoveFile(const char *file)
{
	if (file && isHarmlessPathname(file))
		execCommand(string("rm -rf ") + file);
}

// PingFile
bool
EntryTest::PingFile(const char *path, BEntry *entry)
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

// PingDir
bool
EntryTest::PingDir(const char *path, BEntry *entry)
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

// PingLink
bool
EntryTest::PingLink(const char *path, const char *target, BEntry *entry)
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
			result = (string(linkTarget) == target);
		}
	}
	// check entry
	if (result && entry) {
		BPath entryPath;
		result = (entry->GetPath(&entryPath) == B_OK && entryPath == path);
	}
	return result;
}

// MiscTest
void
EntryTest::MiscTest()
{
	BNode node(file1.cpath);
	BEntry entry(file1.cpath);
		
	CPPUNIT_ASSERT(node.Lock() == B_OK);
	CPPUNIT_ASSERT( entry.Exists() );
}
	


// directory name for too long path name (70 characters)
static const char *tooLongDirname =
	"1234567890123456789012345678901234567890123456789012345678901234567890";

// too long entry name (257 characters)
static const char *tooLongEntryname = 
	"1234567890123456789012345678901234567890123456789012345678901234567890"
	"1234567890123456789012345678901234567890123456789012345678901234567890"
	"1234567890123456789012345678901234567890123456789012345678901234567890"
	"12345678901234567890123456789012345678901234567";


static void
init_entry_test()
{
	// root dir for testing
	testDir.initDir(badTestEntry, "testDir");
	testDir.initPath((string("/tmp/") + testDir.name).c_str());
	allTestEntries.pop_back();
	// other entries
	dir1.initDir(testDir, "dir1");
	dir2.initDir(testDir, "dir2");
	file1.initFile(dir1, "file1");
	file2.init(dir1, "file2", ABSTRACT_ENTRY);
	file3.init(dir2, "file3", ABSTRACT_ENTRY);
	file4.init(dir1, "file4", ABSTRACT_ENTRY);
	subDir1.initDir(dir1, "subDir1");
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
	for (list<TestEntry*>::iterator it = allTestEntries.begin();
		 it != allTestEntries.end(); it++) {
		(*it)->initPath();
	}
	// complete initialization
	testDir.completeInit();
	for (list<TestEntry*>::iterator it = allTestEntries.begin();
		 it != allTestEntries.end(); it++) {
		(*it)->completeInit();
	}
	// create the set up command line
	setUpCommandLine = string("mkdir ") + testDir.path;
	for (list<TestEntry*>::iterator it = allTestEntries.begin();
		 it != allTestEntries.end(); it++) {
		TestEntry *entry = *it;
		string command;
		switch (entry->kind) {
			case ABSTRACT_ENTRY:
				break;
			case DIR_ENTRY:
			{
				if (entry->path.length() < B_PATH_NAME_LENGTH) {
					command = string("mkdir ") + entry->path;
				} else {
					command = string("( ")
						+ "cd " + entry->super->super->path + " ; "
						+ " mkdir " + entry->super->name + "/" + entry->name
						+ " )";
				}
				break;
			}
			case FILE_ENTRY:
			{
				command = string("touch ") + entry->path;
				break;
			}
			case LINK_ENTRY:
			{
				command = string("ln -s ") + entry->link + " " + entry->path;
				break;
			}
			default:
				break;
		}
		if (command.length() > 0) {
			if (setUpCommandLine.length() == 0)
				setUpCommandLine = command;
			else
				setUpCommandLine += string(" ; ") + command;
		}
	}
	// create the tear down command line
	tearDownCommandLine = string("rm -rf ") + testDir.path;
}

struct InitEntryTest {
	InitEntryTest()
	{
		init_entry_test();
	}
} _InitEntryTest;


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

// init
void
TestEntry::init(TestEntry &super, string name, test_entry_kind kind,
				bool relative)
{
	this->super		= &super;
	this->name		= name;
	this->kind		= kind;
	this->relative	= relative;
	allTestEntries.push_back(this);
}

// initDir
void
TestEntry::initDir(TestEntry &super, string name)
{
	init(super, name, DIR_ENTRY);
}

// initFile
void
TestEntry::initFile(TestEntry &super, string name)
{
	init(super, name, FILE_ENTRY);
}

// initRLink
void
TestEntry::initRLink(TestEntry &super, string name, TestEntry &target)
{
	init(super, name, LINK_ENTRY, true);
	this->target = &target;
}

// initALink
void
TestEntry::initALink(TestEntry &super, string name, TestEntry &target)
{
	init(super, name, LINK_ENTRY, false);
	this->target = &target;
}

// initPath
void
TestEntry::initPath(const char *pathName)
{
	if (pathName)
		path = pathName;
	else
		path = super->path + "/" + name;
}

// completeInit
void
TestEntry::completeInit()
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

// get_ref
const entry_ref &
TestEntry::get_ref()
{
	if (isConcrete() || isAbstract())
		get_entry_ref_for_entry(super->cpath, cname, &ref);
	return ref;
}

// get_shortest_relative_path
static
string
get_shortest_relative_path(TestEntry *dir, TestEntry *entry)
{
	string relPath;
	// put all super directories (including dir itself) of the dir in a set
	map<TestEntry*, int> superDirs;
	int dirSuperLevel = 0;
	for (TestEntry *superDir = dir;
		 superDir != &badTestEntry;
		 superDir = superDir->super, dirSuperLevel++) {
		superDirs[superDir] = dirSuperLevel;
	}
	// find the first super dir that dir and entry have in common
	TestEntry *commonSuperDir = &badTestEntry;
	int targetSuperLevel = 0;
	for (TestEntry *superDir = entry;
		 commonSuperDir == &badTestEntry
		 && superDir != &badTestEntry;
		 superDir = superDir->super, targetSuperLevel++) {
		if (superDirs.find(superDir) != superDirs.end())
			commonSuperDir = superDir;
	}
	// construct the relative path
	if (commonSuperDir != &badTestEntry) {
		dirSuperLevel = superDirs[commonSuperDir];
		if (dirSuperLevel == 0 && targetSuperLevel == 0) {
			// entry == dir
			relPath == ".";
		} else {
			// levels down
			for (TestEntry *superDir = entry;
				 superDir != commonSuperDir;
				 superDir = superDir->super) {
				if (relPath.length() == 0)
					relPath = superDir->name;
				else
					relPath = superDir->name + "/" + relPath;
			}
			// levels up
			for (int i = dirSuperLevel; i > 0; i--) {
				if (relPath.length() == 0)
					relPath = "..";
				else
					relPath = string("../") + relPath;
			}
		}
	}
	return relPath;
}

// resolve_link
static
TestEntry *
resolve_link(TestEntry *entry)
{
	set<TestEntry*> followedLinks;
	while (entry != &badTestEntry && entry->kind == LINK_ENTRY) {
		if (followedLinks.find(entry) == followedLinks.end()) {
			followedLinks.insert(entry);
			entry = entry->target;
		} else
			entry = &badTestEntry;	// cyclic link
	}
	return entry;
}




