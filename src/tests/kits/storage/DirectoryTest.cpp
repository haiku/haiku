// DirectoryTest.cpp

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <SymLink.h>

#include "Test.StorageKit.h"
#include "DirectoryTest.h"

// Suite
DirectoryTest::Test*
DirectoryTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<DirectoryTest> TC;
	
	NodeTest::AddBaseClassTests<DirectoryTest>("BDirectory::", suite);

	suite->addTest( new TC("BDirectory::Init Test 1",
						   &DirectoryTest::InitTest1) );
	suite->addTest( new TC("BDirectory::Init Test 2",
						   &DirectoryTest::InitTest2) );
	suite->addTest( new TC("BDirectory::GetEntry Test",
						   &DirectoryTest::GetEntryTest) );
	suite->addTest( new TC("BDirectory::IsRoot Test",
						   &DirectoryTest::IsRootTest) );
	suite->addTest( new TC("BDirectory::FindEntry Test",
						   &DirectoryTest::FindEntryTest) );
	suite->addTest( new TC("BDirectory::Contains Test",
						   &DirectoryTest::ContainsTest) );
	suite->addTest( new TC("BDirectory::GetStatFor Test",
						   &DirectoryTest::GetStatForTest) );
	suite->addTest( new TC("BDirectory::EntryIteration Test",
						   &DirectoryTest::EntryIterationTest) );
	suite->addTest( new TC("BDirectory::Creation Test",
						   &DirectoryTest::EntryCreationTest) );
	suite->addTest( new TC("BDirectory::Assignment Test",
						   &DirectoryTest::AssignmentTest) );
	suite->addTest( new TC("BDirectory::CreateDirectory Test",
						   &DirectoryTest::CreateDirectoryTest) );
	
	return suite;
}		

// CreateRONodes
void
DirectoryTest::CreateRONodes(TestNodes& testEntries)
{
	testEntries.clear();
	const char *filename;
	filename = "/";
	testEntries.add(new BDirectory(filename), filename);
	filename = "/boot";
	testEntries.add(new BDirectory(filename), filename);
	filename = "/boot/home";
	testEntries.add(new BDirectory(filename), filename);
	filename = existingDirname;
	testEntries.add(new BDirectory(filename), filename);
}

// CreateRWNodes
void
DirectoryTest::CreateRWNodes(TestNodes& testEntries)
{
	testEntries.clear();
	const char *filename;
	filename = existingDirname;
	testEntries.add(new BDirectory(filename), filename);
	filename = existingSubDirname;
	testEntries.add(new BDirectory(filename), filename);
}

// CreateUninitializedNodes
void
DirectoryTest::CreateUninitializedNodes(TestNodes& testEntries)
{
	testEntries.clear();
	testEntries.add(new BDirectory, "");
}

// setUp
void DirectoryTest::setUp()
{
	NodeTest::setUp();
}

// tearDown
void DirectoryTest::tearDown()
{
	NodeTest::tearDown();
}

// InitTest1
void
DirectoryTest::InitTest1()
{
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *existing = existingDirname;
	const char *existingSub = existingSubDirname;
	const char *existingRelSub = existingRelSubDirname;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	// 1. default constructor
	nextSubTest();
	{
		BDirectory dir;
		CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	}

	// 2. BDirectory(const char*)
	nextSubTest();
	{
		BDirectory dir(existing);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory dir(nonExisting);
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BDirectory dir((const char *)NULL);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory dir("");
		// R5 returns B_ENTRY_NOT_FOUND instead of B_BAD_VALUE.
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BDirectory dir(existingFile);
		// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory dir(tooLongEntryname);
		CPPUNIT_ASSERT( dir.InitCheck() == B_NAME_TOO_LONG );
	}
	nextSubTest();
	{
		BDirectory dir(fileDirname);
		// R5 returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}

	// 3. BDirectory(const BEntry*)
	nextSubTest();
	{
		BEntry entry(existing);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BDirectory dir(&entry);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BEntry entry(nonExisting);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BDirectory dir(&entry);
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BDirectory dir((BEntry *)NULL);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BEntry entry;
		BDirectory dir(&entry);
		CPPUNIT_ASSERT( equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	}
	nextSubTest();
	{
		BEntry entry(existingFile);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BDirectory dir(&entry);
		// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BEntry entry(tooLongEntryname);
		// R5 returns E2BIG instead of B_NAME_TOO_LONG
		CPPUNIT_ASSERT( equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG) );
		BDirectory dir(&entry);
		CPPUNIT_ASSERT( equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	}

	// 4. BDirectory(const entry_ref*)
	nextSubTest();
	{
		BEntry entry(existing);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BDirectory dir(&ref);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BEntry entry(nonExisting);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BDirectory dir(&ref);
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BDirectory dir((entry_ref *)NULL);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BEntry entry(existingFile);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BDirectory dir(&ref);
		// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}

	// 5. BDirectory(const node_ref*)
	nextSubTest();
	{
		BNode node(existing);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
		node_ref nref;
		CPPUNIT_ASSERT( node.GetNodeRef(&nref) == B_OK );
		BDirectory dir(&nref);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	}
// R5: crashs, when passing a NULL node_ref.
#if !SK_TEST_R5
	nextSubTest();
	{
		BDirectory dir((node_ref *)NULL);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
#endif
	nextSubTest();
	{
		BNode node(existingFile);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
		node_ref nref;
		CPPUNIT_ASSERT( node.GetNodeRef(&nref) == B_OK );
		BDirectory dir(&nref);
		// R5: returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		// OBOS: returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( equals(dir.InitCheck(), B_BAD_VALUE, B_ENTRY_NOT_FOUND) );
	}

	// 6. BDirectory(const BDirectory*, const char*)
	nextSubTest();
	{
		BDirectory pathDir(existing);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, existingRelSub);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory pathDir(existing);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, existingSub);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(nonExistingSuper);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, nonExistingRel);
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BDirectory dir((BDirectory *)NULL, (const char *)NULL);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory dir((BDirectory *)NULL, existingSub);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(existing);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, (const char *)NULL);
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(existing);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, "");
		// This does not fail in R5, but inits the object to pathDir.
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory pathDir(existingSuperFile);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, existingRelFile);
		// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, tooLongRelEntryname);
		CPPUNIT_ASSERT( dir.InitCheck() == B_NAME_TOO_LONG );
	}
	nextSubTest();
	{
		BDirectory pathDir(fileSuperDirname);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BDirectory dir(&pathDir, fileRelDirname);
		// R5 returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	}
}

// InitTest2
void
DirectoryTest::InitTest2()
{
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *existing = existingDirname;
	const char *existingSub = existingSubDirname;
	const char *existingRelSub = existingRelSubDirname;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	BDirectory dir;
	// 2. SetTo(const char*)
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo((const char *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	// R5 returns B_ENTRY_NOT_FOUND instead of B_BAD_VALUE.
	CPPUNIT_ASSERT( dir.SetTo("") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();
	//
	nextSubTest();
	// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( dir.SetTo(existingFile) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(tooLongEntryname) == B_NAME_TOO_LONG );
	CPPUNIT_ASSERT( dir.InitCheck() == B_NAME_TOO_LONG );
	dir.Unset();
	//
	nextSubTest();
	// R5 returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( dir.SetTo(fileDirname) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();

	// 3. BDirectory(const BEntry*)
	nextSubTest();
	BEntry entry(existing);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	dir.Unset();
	//
	nextSubTest();
	entry.SetTo(nonExisting);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&entry) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo((BEntry *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	entry.Unset();
	CPPUNIT_ASSERT( equals(dir.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	dir.Unset();
	//
	nextSubTest();
	entry.SetTo(existingFile);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( dir.SetTo(&entry) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	entry.SetTo(tooLongEntryname);
	// R5 returns E2BIG instead of B_NAME_TOO_LONG
	CPPUNIT_ASSERT( equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG) );
	CPPUNIT_ASSERT( equals(dir.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	dir.Unset();

	// 4. BDirectory(const entry_ref*)
	nextSubTest();
	entry.SetTo(existing);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	entry_ref ref;
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	dir.Unset();
	//
	nextSubTest();
	entry.SetTo(nonExisting);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&ref) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo((entry_ref *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	entry.SetTo(existingFile);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( dir.SetTo(&ref) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();

	// 5. BDirectory(const node_ref*)
	nextSubTest();
	BNode node(existing);
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	node_ref nref;
	CPPUNIT_ASSERT( node.GetNodeRef(&nref) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&nref) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	dir.Unset();
	//
// R5: crashs, when passing a NULL node_ref.
#if !SK_TEST_R5
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo((node_ref *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
#endif
	//
	nextSubTest();
	CPPUNIT_ASSERT( node.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( node.GetNodeRef(&nref) == B_OK );
	// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
	// OBOS: returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( equals(dir.SetTo(&nref), B_BAD_VALUE, B_ENTRY_NOT_FOUND) );
	CPPUNIT_ASSERT( equals(dir.InitCheck(), B_BAD_VALUE, B_ENTRY_NOT_FOUND) );
	dir.Unset();

	// 6. BDirectory(const BDirectory*, const char*)
	nextSubTest();
	BDirectory pathDir(existing);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, existingRelSub) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(existing);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, existingSub) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(nonExistingSuper);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, nonExistingRel) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo((BDirectory *)NULL, (const char *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo((BDirectory *)NULL, existingSub) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(existing);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, (const char *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(existing);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	// This does not fail in R5, but inits the object to pathDir.
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, "") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(existingSuperFile);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	// R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, existingRelFile) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.InitCheck() == B_BAD_VALUE );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(tooLongSuperEntryname);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, tooLongRelEntryname)
					== B_NAME_TOO_LONG );
	CPPUNIT_ASSERT( dir.InitCheck() == B_NAME_TOO_LONG );
	dir.Unset();
	//
	nextSubTest();
	pathDir.SetTo(fileSuperDirname);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	// R5 returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
	CPPUNIT_ASSERT( dir.SetTo(&pathDir, fileRelDirname) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	dir.Unset();
}

// GetEntryTest
void
DirectoryTest::GetEntryTest()
{
	const char *existing = existingDirname;
	const char *nonExisting = nonExistingDirname;
	//
	nextSubTest();
	BDirectory dir;
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	BEntry entry;
	CPPUNIT_ASSERT( dir.GetEntry(&entry) == B_NO_INIT );
	dir.Unset();
	entry.Unset();
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.GetEntry(&entry) == B_OK );
	CPPUNIT_ASSERT( entry == BEntry(existing) );
	dir.Unset();
	entry.Unset();
	//
#if !SK_TEST_R5
// R5: crashs, when passing a NULL BEntry.
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.GetEntry((BEntry *)NULL) == B_BAD_VALUE );
	dir.Unset();
	entry.Unset();
#endif
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.GetEntry(&entry) == B_NO_INIT );
	dir.Unset();
	entry.Unset();
}

// IsRootTest
void
DirectoryTest::IsRootTest()
{
	//
	nextSubTest();
	BDirectory dir;
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.IsRootDirectory() == false );
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/boot") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.IsRootDirectory() == true );
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/boot/beos") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.IsRootDirectory() == false );
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/tmp") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.IsRootDirectory() == false );
	//
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.IsRootDirectory() == true );
}

// FindEntryTest
void
DirectoryTest::FindEntryTest()
{
	const char *existingFile = existingFilename;
	const char *existing = existingDirname;
	const char *existingSub = existingSubDirname;
	const char *existingRelSub = existingRelSubDirname;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	const char *dirLink = dirLinkname;
	const char *badLink = badLinkname;
	const char *cyclicLink1 = cyclicLinkname1;
	// existing absolute path, uninitialized BDirectory
	nextSubTest();
	BDirectory dir;
	BEntry entry;
	BPath path;
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(existing, &entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == existing == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// existing absolute path, badly initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.FindEntry(existing, &entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == existing == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// existing path relative to current dir, uninitialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	chdir(existing);
	CPPUNIT_ASSERT( dir.FindEntry(existingRelSub, &entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == existingSub == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	chdir("/");
	// existing path relative to current dir,
	// initialized BDirectory != current dir
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSub) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	chdir(existing);
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( dir.FindEntry(existingRelSub, &entry) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	dir.Unset();
	entry.Unset();
	path.Unset();
	chdir("/");
	// abstract entry
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExistingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	chdir(existing);
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( dir.FindEntry(nonExistingRel, &entry) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	dir.Unset();
	entry.Unset();
	path.Unset();
	chdir("/");
	// bad args
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
// R5: crashs, when passing a NULL BEntry.
#if !SK_TEST_R5
	CPPUNIT_ASSERT( dir.FindEntry(existingRelSub, NULL) == B_BAD_VALUE );
#endif
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( dir.FindEntry(NULL, &entry) == B_BAD_VALUE );
	CPPUNIT_ASSERT( equals(entry.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
// R5: crashs, when passing a NULL BEntry.
#if !SK_TEST_R5
	CPPUNIT_ASSERT( dir.FindEntry(NULL, NULL) == B_BAD_VALUE );
#endif
	dir.Unset();
	entry.Unset();
	path.Unset();
	// don't traverse a valid link
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(dirLink, &entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == dirLink == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// traverse a valid link
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(dirLink, &entry, true) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == existing == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// don't traverse an invalid link
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(badLink, &entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == badLink == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// traverse an invalid link
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(badLink, &entry, true) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( equals(entry.InitCheck(), B_ENTRY_NOT_FOUND, B_NO_INIT) );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// don't traverse a cyclic link
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(cyclicLink1, &entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
	CPPUNIT_ASSERT( path == cyclicLink1 == B_OK );
	dir.Unset();
	entry.Unset();
	path.Unset();
	// traverse a cyclic link
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.FindEntry(cyclicLink1, &entry, true) == B_LINK_LIMIT );
	CPPUNIT_ASSERT( entry.InitCheck() == B_LINK_LIMIT );
	dir.Unset();
	entry.Unset();
	path.Unset();
}

// ContainsTest
void
DirectoryTest::ContainsTest()
{
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *existing = existingDirname;
	const char *existingSuper = existingSuperDirname;
	const char *existingSub = existingSubDirname;
	const char *existingRelSub = existingRelSubDirname;
	const char *nonExisting = nonExistingDirname;
	const char *dirLink = dirLinkname;
	const char *dirSuperLink = dirSuperLinkname;
	// 1. Contains(const char *, int32)
	// existing entry, initialized BDirectory
	nextSubTest();
	BDirectory dir(existing);
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingSub) == true );
	dir.Unset();
	// existing entry, uninitialized BDirectory
	// R5 returns true!
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.Contains(existing) == true );
	dir.Unset();
	// non-existing entry, uninitialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.Contains(nonExisting) == false );
	dir.Unset();
	// existing entry, badly initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.Contains(existing) == true );
	dir.Unset();
	// non-existing entry, badly initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.Contains(nonExisting) == false );
	dir.Unset();
	// initialized BDirectory, bad args
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains((const char*)NULL) == true );
	dir.Unset();
	// uninitialized BDirectory, bad args
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.Contains((const char*)NULL) == false );
	dir.Unset();
	// existing entry (second level, absolute path), initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingSub) == true );
	dir.Unset();
	// existing entry (second level, name only), initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingRelSub) == false );
	dir.Unset();
	// initialized BDirectory, self containing
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existing) == true );
	dir.Unset();
	// existing entry (dir), initialized BDirectory, matching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existing, B_DIRECTORY_NODE) == true );
	dir.Unset();
	// existing entry (dir), initialized BDirectory, mismatching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existing, B_FILE_NODE) == false );
	dir.Unset();
	// existing entry (file), initialized BDirectory, matching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingFile, B_FILE_NODE) == true );
	dir.Unset();
	// existing entry (file), initialized BDirectory, mismatching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingFile, B_SYMLINK_NODE) == false );
	dir.Unset();
	// existing entry (link), initialized BDirectory, matching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(dirLink, B_SYMLINK_NODE) == true );
	dir.Unset();
	// existing entry (link), initialized BDirectory, mismatching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(dirLink, B_DIRECTORY_NODE) == false );
	dir.Unset();
	// existing entry (relative path), initialized BDirectory,
	// matching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingRelFile, B_FILE_NODE) == true );
	dir.Unset();
	// existing entry (relative path), initialized BDirectory,
	// mismatching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(existingRelFile, B_SYMLINK_NODE) == false );
	dir.Unset();

	// 2. Contains(const BEntry *, int32)
	// existing entry, initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	BEntry entry(existingSub);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains(&entry) == true );
	dir.Unset();
	// existing entry, uninitialized BDirectory
	// R5: unlike the other version, this one returns false
	// OBOS: both versions return true
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( equals(dir.Contains(&entry), false, true) );
	dir.Unset();
	entry.Unset();
	// non-existing entry, uninitialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( entry.SetTo(nonExisting) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry) == false );
	dir.Unset();
	entry.Unset();
	// existing entry, badly initialized BDirectory
	// R5: unlike the other version, this one returns false
	// OBOS: both versions return true
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK);
	CPPUNIT_ASSERT( equals(dir.Contains(&entry), false, true) );
	dir.Unset();
	entry.Unset();
	// non-existing entry, badly initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( entry.SetTo(nonExisting) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry) == false );
	dir.Unset();
	entry.Unset();
	// initialized BDirectory, bad args
// R5 crashs, when passing a NULL BEntry
#if !SK_TEST_R5
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.Contains((const BEntry*)NULL) == false );
	dir.Unset();
#endif
	// uninitialized BDirectory, bad args
	nextSubTest();
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.Contains((const BEntry*)NULL) == false );
	dir.Unset();
	// existing entry (second level, absolute path), initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existingSub) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry) == true );
	dir.Unset();
	entry.Unset();
	// initialized BDirectory, self containing
	// R5: behavior is different from Contains(const char*)
	// OBOS: both versions return true
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK);
	CPPUNIT_ASSERT( equals(dir.Contains(&entry), false, true) );
	dir.Unset();
	entry.Unset();
	// existing entry (dir), initialized BDirectory, matching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry, B_DIRECTORY_NODE) == true );
	dir.Unset();
	entry.Unset();
	// existing entry (dir), initialized BDirectory, mismatching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry, B_FILE_NODE) == false );
	dir.Unset();
	entry.Unset();
	// existing entry (file), initialized BDirectory, matching node kind
// R5 bug: returns false
#if !SK_TEST_R5
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry, B_FILE_NODE) == true );
	dir.Unset();
	entry.Unset();
#endif
	// existing entry (file), initialized BDirectory, mismatching node kind
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry, B_SYMLINK_NODE) == false );
	dir.Unset();
	entry.Unset();
	// existing entry (link), initialized BDirectory, matching node kind
// R5 bug: returns false
#if !SK_TEST_R5
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(dirLink) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry, B_SYMLINK_NODE) == true );
	dir.Unset();
	entry.Unset();
#endif
	// existing entry (link), initialized BDirectory, mismatching node kind
// R5 bug: returns true
#if !SK_TEST_R5
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(dirLink) == B_OK);
	CPPUNIT_ASSERT( dir.Contains(&entry, B_DIRECTORY_NODE) == false );
	dir.Unset();
	entry.Unset();
#endif
}

// GetStatForTest
void
DirectoryTest::GetStatForTest()
{
	const char *existing = existingDirname;
	const char *existingSuper = existingSuperDirname;
	const char *existingRel = existingRelDirname;
	const char *nonExisting = nonExistingDirname;
	// uninitialized dir, existing entry, absolute path
	nextSubTest();
	BDirectory dir;
	BEntry entry;
	struct stat stat1, stat2;
	memset(&stat1, 0, sizeof(struct stat));
	memset(&stat2, 0, sizeof(struct stat));
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( dir.GetStatFor(existing, &stat1) == B_NO_INIT );
	dir.Unset();
	entry.Unset();
	// badly initialized dir, existing entry, absolute path
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.InitCheck() == B_ENTRY_NOT_FOUND );
	memset(&stat1, 0, sizeof(struct stat));
	memset(&stat2, 0, sizeof(struct stat));
	CPPUNIT_ASSERT( dir.GetStatFor(existing, &stat1) == B_NO_INIT );
	dir.Unset();
	entry.Unset();
	// initialized dir, existing entry, absolute path
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	memset(&stat1, 0, sizeof(struct stat));
	memset(&stat2, 0, sizeof(struct stat));
	CPPUNIT_ASSERT( dir.GetStatFor(existing, &stat1) == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( entry.GetStat(&stat2) == B_OK );
	CPPUNIT_ASSERT( stat1 == stat2 );
	dir.Unset();
	entry.Unset();
	// initialized dir, existing entry, relative path
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(existingSuper) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	memset(&stat1, 0, sizeof(struct stat));
	memset(&stat2, 0, sizeof(struct stat));
	CPPUNIT_ASSERT( dir.GetStatFor(existingRel, &stat1) == B_OK );
	CPPUNIT_ASSERT( entry.SetTo(existing) == B_OK );
	CPPUNIT_ASSERT( entry.GetStat(&stat2) == B_OK );
	CPPUNIT_ASSERT( stat1 == stat2 );
	dir.Unset();
	entry.Unset();
	// initialized dir, non-existing entry, absolute path
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	memset(&stat1, 0, sizeof(struct stat));
	memset(&stat2, 0, sizeof(struct stat));
	CPPUNIT_ASSERT( dir.GetStatFor(nonExisting, &stat1) == B_ENTRY_NOT_FOUND );
	dir.Unset();
	entry.Unset();
	// initialized dir, bad args (NULL path)
	// R5 returns B_OK and the stat structure for the directory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	memset(&stat1, 0, sizeof(struct stat));
	memset(&stat2, 0, sizeof(struct stat));
	CPPUNIT_ASSERT( dir.GetStatFor(NULL, &stat1) == B_OK );
	CPPUNIT_ASSERT( entry.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( entry.GetStat(&stat2) == B_OK );
	CPPUNIT_ASSERT( stat1 == stat2 );
	dir.Unset();
	entry.Unset();
	// initialized dir, bad args (empty path)
	// R5 returns B_ENTRY_NOT_FOUND
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( dir.GetStatFor("", &stat1) == B_ENTRY_NOT_FOUND );
	dir.Unset();
	entry.Unset();
	// initialized dir, bad args
	// R5 returns B_BAD_ADDRESS instead of B_BAD_VALUE
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo("/") == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( equals(dir.GetStatFor(existing, NULL), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(dir.GetStatFor(NULL, NULL), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	dir.Unset();
	entry.Unset();
}

// EntryIterationTest
void
DirectoryTest::EntryIterationTest()
{
	const char *existingFile = existingFilename;
	const char *nonExisting = nonExistingDirname;
	const char *testDir1 = testDirname1;
	// create a test directory
	execCommand(string("mkdir ") + testDir1);
	// 1. empty directory
	TestSet testSet;
	testSet.add(".");
	testSet.add("..");
	// GetNextEntry
	nextSubTest();
	BDirectory dir(testDir1);
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	BEntry entry;
	CPPUNIT_ASSERT( dir.GetNextEntry(&entry) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	// GetNextRef
	nextSubTest();
	entry_ref ref;
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	CPPUNIT_ASSERT( dir.GetNextRef(&ref) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	// GetNextDirents
	nextSubTest();
	size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
	char buffer[bufSize];
	dirent *ents = (dirent *)buffer;
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	while (dir.GetNextDirents(ents, bufSize, 1) == 1)
		CPPUNIT_ASSERT( testSet.test(ents->d_name) == true );
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	testSet.rewind();
	// CountEntries
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	CPPUNIT_ASSERT( dir.CountEntries() == 0 );
	dir.Unset();

	// 2. non-empty directory
	string dirPathName(string(testDir1) + "/");
	string entryName("file1");
	execCommand(string("touch ") + dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("file2");
	execCommand(string("touch ") + dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("file3");
	execCommand(string("touch ") + dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("dir1");
	execCommand(string("mkdir ") + dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("dir2");
	execCommand(string("mkdir ") + dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("dir3");
	execCommand(string("mkdir ") + dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("link1");
	execCommand(string("ln -s ") + existingFile + " "
				+ dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("link2");
	execCommand(string("ln -s ") + existingFile + " "
				+ dirPathName + entryName);
	testSet.add(entryName);
	entryName = ("link3");
	execCommand(string("ln -s ") + existingFile + " "
				+ dirPathName + entryName);
	testSet.add(entryName);
	// GetNextEntry
	nextSubTest();
	testSet.test(".");
	testSet.test("..");
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
		CPPUNIT_ASSERT( testSet.test(path.Leaf()) == true );
	}
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	testSet.rewind();
	// GetNextRef
	nextSubTest();
	testSet.test(".");
	testSet.test("..");
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	while (dir.GetNextRef(&ref) == B_OK)
		CPPUNIT_ASSERT( testSet.test(ref.name) == true );
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	testSet.rewind();
	// GetNextDirents
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	while (dir.GetNextDirents(ents, bufSize, 1) == 1)
		CPPUNIT_ASSERT( testSet.test(ents->d_name) == true );
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	testSet.rewind();
	// CountEntries
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	CPPUNIT_ASSERT( dir.CountEntries() == 9 );
	CPPUNIT_ASSERT( dir.GetNextRef(&ref) == B_OK );
	CPPUNIT_ASSERT( dir.CountEntries() == 9 );
	dir.Unset();

	// 3. interleaving use of the different methods
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	while (dir.GetNextDirents(ents, bufSize, 1) == 1) {
		CPPUNIT_ASSERT( testSet.test(ents->d_name) == true );
		if (dir.GetNextRef(&ref) == B_OK)
			CPPUNIT_ASSERT( testSet.test(ref.name) == true );
		if (dir.GetNextEntry(&entry) == B_OK) {
			BPath path;
			CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
			CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
			CPPUNIT_ASSERT( testSet.test(path.Leaf()) == true );
		}
	}
	testSet.test(".", false);	// in case they have been skipped
	testSet.test("..", false);	//
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( dir.Rewind() == B_OK );
	dir.Unset();
	entry.Unset();
	testSet.rewind();

	// 4. uninitialized BDirectory
	nextSubTest();
	dir.Unset();
	// R5: unlike the others GetNextRef() returns B_NO_INIT
	CPPUNIT_ASSERT( dir.GetNextEntry(&entry) == B_FILE_ERROR );
	CPPUNIT_ASSERT( equals(dir.GetNextRef(&ref), B_NO_INIT, B_FILE_ERROR) );
	CPPUNIT_ASSERT( dir.Rewind() == B_FILE_ERROR );
	CPPUNIT_ASSERT( dir.GetNextDirents(ents, bufSize, 1) == B_FILE_ERROR );
	CPPUNIT_ASSERT( dir.CountEntries() == B_FILE_ERROR );
	dir.Unset();

	// 5. badly initialized BDirectory
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	// R5: unlike the others GetNextRef() returns B_NO_INIT
	CPPUNIT_ASSERT( dir.GetNextEntry(&entry) == B_FILE_ERROR );
	CPPUNIT_ASSERT( equals(dir.GetNextRef(&ref), B_NO_INIT, B_FILE_ERROR) );
	CPPUNIT_ASSERT( dir.Rewind() == B_FILE_ERROR );
	CPPUNIT_ASSERT( dir.GetNextDirents(ents, bufSize, 1) == B_FILE_ERROR );
	CPPUNIT_ASSERT( dir.CountEntries() == B_FILE_ERROR );
	dir.Unset();

	// 6. bad args
// R5 crashs, when passing a NULL BEntry or entry_ref
#if !SK_TEST_R5
	nextSubTest();
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	CPPUNIT_ASSERT( dir.GetNextEntry(NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( dir.GetNextRef(NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( equals(dir.GetNextDirents(NULL, bufSize, 1),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	dir.Unset();
#endif

	// 7. link traversation
	nextSubTest();
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	entryName = ("link1");
	execCommand(string("ln -s ") + existingFile + " "
				+ dirPathName + entryName);
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	CPPUNIT_ASSERT( dir.GetNextEntry(&entry, true) == B_OK );
	BPath path;
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	BEntry entry2(existingFile);
	CPPUNIT_ASSERT( entry2.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry == entry2 );
	dir.Unset();
	entry.Unset();
}

// EntryCreationTest
void
DirectoryTest::EntryCreationTest()
{
	const char *existingFile = existingFilename;
	const char *existing = existingDirname;
	const char *testDir1 = testDirname1;
	// create a test directory
	execCommand(string("mkdir ") + testDir1);
	// 1. relative path
	BDirectory dir(testDir1);
	CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
	// CreateDirectory
	// dir doesn't already exist
	nextSubTest();
	BDirectory subdir;
	string dirPathName(string(testDir1) + "/");
	string entryName("subdir1");
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir) == B_OK );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_OK );
	BEntry entry;
	CPPUNIT_ASSERT( subdir.GetEntry(&entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry == BEntry((dirPathName + entryName).c_str()) );
	subdir.Unset();
	CPPUNIT_ASSERT( subdir.SetTo((dirPathName + entryName).c_str()) == B_OK );
	subdir.Unset();
	// dir does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_NO_INIT );
	subdir.Unset();
	// CreateFile
	// file doesn't already exist
	nextSubTest();
	BFile file;
	entryName = "file1";
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo((dirPathName + entryName).c_str(),
							   B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, don't fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, false) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo((dirPathName + entryName).c_str(),
							   B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
	file.Unset();
	// CreateSymLink
	// link doesn't already exist
	nextSubTest();
	BSymLink link;
	entryName = "link1";
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	link.Unset();
	CPPUNIT_ASSERT( link.SetTo((dirPathName + entryName).c_str()) == B_OK );
	link.Unset();
	// link does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	link.Unset();

	// 2. absolute path
	dir.Unset();
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	CPPUNIT_ASSERT( dir.SetTo(existing) == B_OK );
	// CreateDirectory
	// dir doesn't already exist
	nextSubTest();
	entryName = dirPathName + "subdir1";
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir) == B_OK );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( subdir.GetEntry(&entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry == BEntry(entryName.c_str()) );
	subdir.Unset();
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// dir does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_NO_INIT );
	subdir.Unset();
	// CreateFile
	// file doesn't already exist
	nextSubTest();
	entryName = dirPathName + "file1";
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo(entryName.c_str(), B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, don't fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, false) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo(entryName.c_str(), B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
	file.Unset();
	// CreateSymLink
	// link doesn't already exist
	nextSubTest();
	entryName = dirPathName + "link1";
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	link.Unset();
	CPPUNIT_ASSERT( link.SetTo(entryName.c_str()) == B_OK );
	link.Unset();
	// link does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	link.Unset();

	// 3. uninitialized BDirectory, absolute path
	dir.Unset();
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	// CreateDirectory
	// dir doesn't already exist
	nextSubTest();
	entryName = dirPathName + "subdir1";
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir) == B_OK );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( subdir.GetEntry(&entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry == BEntry(entryName.c_str()) );
	subdir.Unset();
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// dir does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_NO_INIT );
	subdir.Unset();
	// CreateFile
	// file doesn't already exist
	nextSubTest();
	entryName = dirPathName + "file1";
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo(entryName.c_str(), B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, don't fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, false) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo(entryName.c_str(), B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
	file.Unset();
	// CreateSymLink
	// link doesn't already exist
	nextSubTest();
	entryName = dirPathName + "link1";
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	link.Unset();
	CPPUNIT_ASSERT( link.SetTo(entryName.c_str()) == B_OK );
	link.Unset();
	// link does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	link.Unset();

	// 4. uninitialized BDirectory, relative path, current directory
	dir.Unset();
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	chdir(testDir1);
	// CreateDirectory
	// dir doesn't already exist
	nextSubTest();
	entryName = "subdir1";
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir) == B_OK );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( subdir.GetEntry(&entry) == B_OK );
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entry == BEntry((dirPathName + entryName).c_str()) );
	subdir.Unset();
	CPPUNIT_ASSERT( subdir.SetTo((dirPathName + entryName).c_str()) == B_OK );
	subdir.Unset();
	// dir does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), &subdir)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_NO_INIT );
	subdir.Unset();
	// CreateFile
	// file doesn't already exist
	nextSubTest();
	entryName = "file1";
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo((dirPathName + entryName).c_str(),
							   B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, don't fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, false) == B_OK );
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	file.Unset();
	CPPUNIT_ASSERT( file.SetTo((dirPathName + entryName).c_str(),
							   B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), &file, true)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
	file.Unset();
	// CreateSymLink
	// link doesn't already exist
	nextSubTest();
	entryName = "link1";
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	link.Unset();
	CPPUNIT_ASSERT( link.SetTo((dirPathName + entryName).c_str()) == B_OK );
	link.Unset();
	// link does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, &link)
					== B_FILE_EXISTS );
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	link.Unset();
	chdir("/");

	// 5. bad args
	dir.Unset();
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	CPPUNIT_ASSERT( dir.SetTo(testDir1) == B_OK );
	// CreateDirectory
	nextSubTest();
	entryName = "subdir1";
	CPPUNIT_ASSERT( equals(dir.CreateDirectory(NULL, &subdir),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( subdir.InitCheck() == B_NO_INIT );
	subdir.Unset();
	// CreateFile
	// R5: unlike CreateDirectory/SymLink() CreateFile() returns
	//	   B_ENTRY_NOT_FOUND
	nextSubTest();
	entryName = "file1";
	CPPUNIT_ASSERT( equals(dir.CreateFile(NULL, &file), B_ENTRY_NOT_FOUND,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
	file.Unset();
	// CreateSymLink
	nextSubTest();
	entryName = "link1";
	CPPUNIT_ASSERT( equals(dir.CreateSymLink(NULL, existingFile, &link),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( equals(dir.CreateSymLink(entryName.c_str(), NULL, &link),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	link.Unset();

	// 6. uninitialized BDirectory, absolute path, no second param
	dir.Unset();
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
	// CreateDirectory
	// dir doesn't already exist
	nextSubTest();
	entryName = dirPathName + "subdir1";
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), NULL) == B_OK );
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// dir does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateDirectory(entryName.c_str(), NULL)
					== B_FILE_EXISTS );
	subdir.Unset();
	// CreateFile
	// file doesn't already exist
	nextSubTest();
	entryName = dirPathName + "file1";
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), NULL, true) == B_OK );
	CPPUNIT_ASSERT( file.SetTo(entryName.c_str(), B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, don't fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), NULL, false) == B_OK );
	CPPUNIT_ASSERT( file.SetTo(entryName.c_str(), B_READ_ONLY) == B_OK );
	file.Unset();
	// file does already exist, fail
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateFile(entryName.c_str(), NULL, true)
					== B_FILE_EXISTS );
	// CreateSymLink
	// link doesn't already exist
	nextSubTest();
	entryName = dirPathName + "link1";
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, NULL)
					== B_OK );
	CPPUNIT_ASSERT( link.SetTo(entryName.c_str()) == B_OK );
	link.Unset();
	// link does already exist
	nextSubTest();
	CPPUNIT_ASSERT( dir.CreateSymLink(entryName.c_str(), existingFile, NULL)
					== B_FILE_EXISTS );
}

// AssignmentTest
void
DirectoryTest::AssignmentTest()
{
	const char *existing = existingDirname;
	// 1. copy constructor
	// uninitialized
	nextSubTest();
	{
		BDirectory dir;
		CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT );
		BDirectory dir2(dir);
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(dir2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	// existing dir
	nextSubTest();
	{
		BDirectory dir(existing);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
		BDirectory dir2(dir);
		CPPUNIT_ASSERT( dir2.InitCheck() == B_OK );
	}

	// 2. assignment operator
	// uninitialized
	nextSubTest();
	{
		BDirectory dir;
		BDirectory dir2;
		dir2 = dir;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(dir2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	nextSubTest();
	{
		BDirectory dir;
		BDirectory dir2(existing);
		CPPUNIT_ASSERT( dir2.InitCheck() == B_OK );
		dir2 = dir;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(dir2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	// existing dir
	nextSubTest();
	{
		BDirectory dir(existing);
		CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
		BDirectory dir2;
		dir2 = dir;
		CPPUNIT_ASSERT( dir2.InitCheck() == B_OK );
	}
}

// CreateDirectoryTest
void
DirectoryTest::CreateDirectoryTest()
{
	const char *existingFile = existingFilename;
	const char *testDir1 = testDirname1;
	const char *dirLink = dirLinkname;
	const char *fileLink = fileLinkname;
	// 1. absolute path
	execCommand(string("mkdir ") + testDir1);
	// two levels
	nextSubTest();
	string dirPathName(string(testDir1) + "/");
	string entryName(dirPathName + "subdir1/subdir1.1");
	CPPUNIT_ASSERT( create_directory(entryName.c_str(), 0x1ff) == B_OK );
	BDirectory subdir;
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// one level
	nextSubTest();
	entryName = dirPathName + "subdir2";
	CPPUNIT_ASSERT( create_directory(entryName.c_str(), 0x1ff) == B_OK );
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// existing dir
	nextSubTest();
	entryName = dirPathName;
	CPPUNIT_ASSERT( create_directory(entryName.c_str(), 0x1ff) == B_OK );
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();

	// 2. relative path
	execCommand(string("rm -rf ") + testDir1);
	execCommand(string("mkdir ") + testDir1);
	chdir(testDir1);
	// two levels
	nextSubTest();
	entryName = "subdir1/subdir1.1";
	CPPUNIT_ASSERT( create_directory(entryName.c_str(), 0x1ff) == B_OK );
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// one level
	nextSubTest();
	entryName = "subdir2";
	CPPUNIT_ASSERT( create_directory(entryName.c_str(), 0x1ff) == B_OK );
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	// existing dir
	nextSubTest();
	entryName = ".";
	CPPUNIT_ASSERT( create_directory(entryName.c_str(), 0x1ff) == B_OK );
	CPPUNIT_ASSERT( subdir.SetTo(entryName.c_str()) == B_OK );
	subdir.Unset();
	chdir("/");

	// 3. error cases
	// existing file/link
	nextSubTest();
	CPPUNIT_ASSERT( equals(create_directory(existingFile, 0x1ff), B_BAD_VALUE,
						   B_NOT_A_DIRECTORY) );
	CPPUNIT_ASSERT( equals(create_directory(fileLink, 0x1ff), B_BAD_VALUE,
						   B_NOT_A_DIRECTORY) );
	CPPUNIT_ASSERT( create_directory(dirLink, 0x1ff) == B_OK );
	// bad args
	nextSubTest();
	CPPUNIT_ASSERT( create_directory(NULL, 0x1ff) == B_BAD_VALUE );
}

