// SymLinkTest.cpp

#include <stdio.h>
#include <string>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <SymLink.h>

#include "Test.StorageKit.h"
#include "SymLinkTest.h"

// Suite
SymLinkTest::Test*
SymLinkTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<SymLinkTest> TC;
	
	NodeTest::AddBaseClassTests<SymLinkTest>("BSymLink::", suite);

	suite->addTest( new TC("BSymLink::Init Test 1", &SymLinkTest::InitTest1) );
	suite->addTest( new TC("BSymLink::Init Test 2", &SymLinkTest::InitTest2) );
	suite->addTest( new TC("BSymLink::ReadLink Test",
						   &SymLinkTest::ReadLinkTest) );
	suite->addTest( new TC("BSymLink::MakeLinkedPath Test",
						   &SymLinkTest::MakeLinkedPathTest) );
	suite->addTest( new TC("BSymLink::IsAbsolute Test",
						   &SymLinkTest::IsAbsoluteTest) );
	suite->addTest( new TC("BSymLink::Assignment Test",
						   &SymLinkTest::AssignmentTest) );
	
	return suite;
}		

// CreateRONodes
void
SymLinkTest::CreateRONodes(TestNodes& testEntries)
{
	testEntries.clear();
	const char *filename;
	filename = "/tmp";
	testEntries.add(new BSymLink(filename), filename);
	filename = dirLinkname;
	testEntries.add(new BSymLink(filename), filename);
	filename = fileLinkname;
	testEntries.add(new BSymLink(filename), filename);
	filename = badLinkname;
	testEntries.add(new BSymLink(filename), filename);
	filename = cyclicLinkname1;
	testEntries.add(new BSymLink(filename), filename);
}

// CreateRWNodes
void
SymLinkTest::CreateRWNodes(TestNodes& testEntries)
{
	testEntries.clear();
	const char *filename;
	filename = dirLinkname;
	testEntries.add(new BSymLink(filename), filename);
	filename = fileLinkname;
	testEntries.add(new BSymLink(filename), filename);
}

// CreateUninitializedNodes
void
SymLinkTest::CreateUninitializedNodes(TestNodes& testEntries)
{
	testEntries.clear();
	testEntries.add(new BSymLink, "");
}

// setUp
void SymLinkTest::setUp()
{
	NodeTest::setUp();
}

// tearDown
void SymLinkTest::tearDown()
{
	NodeTest::tearDown();
}

// InitTest1
void
SymLinkTest::InitTest1()
{
	const char *dirLink = dirLinkname;
	const char *dirSuperLink = dirSuperLinkname;
	const char *dirRelLink = dirRelLinkname;
	const char *fileLink = fileLinkname;
	const char *existingDir = existingDirname;
	const char *existingSuperDir = existingSuperDirname;
	const char *existingRelDir = existingRelDirname;
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	// 1. default constructor
	nextSubTest();
	{
		BSymLink link;
		CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	}

	// 2. BSymLink(const char*)
	nextSubTest();
	{
		BSymLink link(fileLink);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BSymLink link(nonExisting);
		CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BSymLink link((const char *)NULL);
		CPPUNIT_ASSERT( equals(link.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	nextSubTest();
	{
		BSymLink link("");
		CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BSymLink link(existingFile);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BSymLink link(existingDir);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BSymLink link(tooLongEntryname);
		CPPUNIT_ASSERT( link.InitCheck() == B_NAME_TOO_LONG );
	}

	// 3. BSymLink(const BEntry*)
	nextSubTest();
	{
		BEntry entry(dirLink);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BSymLink link(&entry);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BEntry entry(nonExisting);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BSymLink link(&entry);
		CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BSymLink link((BEntry *)NULL);
		CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BEntry entry;
		BSymLink link(&entry);
		CPPUNIT_ASSERT( equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	}
	nextSubTest();
	{
		BEntry entry(existingFile);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BSymLink link(&entry);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );

	}
	nextSubTest();
	{
		BEntry entry(existingDir);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BSymLink link(&entry);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );

	}
	nextSubTest();
	{
		BEntry entry(tooLongEntryname);
		// R5 returns E2BIG instead of B_NAME_TOO_LONG
		CPPUNIT_ASSERT( equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG) );
		BSymLink link(&entry);
		CPPUNIT_ASSERT( equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	}

	// 4. BSymLink(const entry_ref*)
	nextSubTest();
	{
		BEntry entry(dirLink);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BSymLink link(&ref);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BEntry entry(nonExisting);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BSymLink link(&ref);
		CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BSymLink link((entry_ref *)NULL);
		CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BEntry entry(existingFile);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BSymLink link(&ref);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BEntry entry(existingDir);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BSymLink link(&ref);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}

	// 5. BSymLink(const BDirectory*, const char*)
	nextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, dirRelLink);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, dirLink);
		CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(nonExistingSuper);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, nonExistingRel);
		CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	nextSubTest();
	{
		BSymLink link((BDirectory *)NULL, (const char *)NULL);
		CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BSymLink link((BDirectory *)NULL, dirLink);
		CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, (const char *)NULL);
		CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	}
	nextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, "");
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory pathDir(existingSuperFile);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, existingRelFile);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory pathDir(existingSuperDir);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, existingRelDir);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	}
	nextSubTest();
	{
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, tooLongRelEntryname);
		CPPUNIT_ASSERT( link.InitCheck() == B_NAME_TOO_LONG );
	}
	nextSubTest();
	{
		BDirectory pathDir(fileSuperDirname);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BSymLink link(&pathDir, fileRelDirname);
		CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	}
}

// InitTest2
void
SymLinkTest::InitTest2()
{
	const char *dirLink = dirLinkname;
	const char *dirSuperLink = dirSuperLinkname;
	const char *dirRelLink = dirRelLinkname;
	const char *fileLink = fileLinkname;
	const char *existingDir = existingDirname;
	const char *existingSuperDir = existingSuperDirname;
	const char *existingRelDir = existingRelDirname;
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	BSymLink link;
	// 2. BSymLink(const char*)
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(fileLink) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	nextSubTest();
	CPPUNIT_ASSERT( equals(link.SetTo((const char *)NULL), B_BAD_VALUE,
						   B_NO_INIT) );
	CPPUNIT_ASSERT( equals(link.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo("") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(tooLongEntryname) == B_NAME_TOO_LONG );
	CPPUNIT_ASSERT( link.InitCheck() == B_NAME_TOO_LONG );

	// 3. BSymLink(const BEntry*)
	nextSubTest();
	BEntry entry(dirLink);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(nonExisting) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&entry) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo((BEntry *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	//
	nextSubTest();
	entry.Unset();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( equals(link.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	//
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	// R5 returns E2BIG instead of B_NAME_TOO_LONG
	CPPUNIT_ASSERT( equals(entry.SetTo(tooLongEntryname), E2BIG,
						   B_NAME_TOO_LONG) );
	CPPUNIT_ASSERT( equals(link.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );

	// 4. BSymLink(const entry_ref*)
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(dirLink) == B_OK );
	entry_ref ref;
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(nonExisting) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&ref) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo((entry_ref *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	//
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );

	// 5. BSymLink(const BDirectory*, const char*)
	nextSubTest();
	BDirectory pathDir(dirSuperLink);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, dirRelLink) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, dirLink) == B_BAD_VALUE );
	CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(nonExistingSuper) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, nonExistingRel) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo((BDirectory *)NULL, (const char *)NULL)
					== B_BAD_VALUE );
	CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	//
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo((BDirectory *)NULL, dirLink) == B_BAD_VALUE );
	CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, (const char *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( link.InitCheck() == B_BAD_VALUE );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, "") == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, existingRelFile) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(existingSuperDir) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, existingRelDir) == B_OK );
	CPPUNIT_ASSERT( link.InitCheck() == B_OK );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(tooLongSuperEntryname) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, tooLongRelEntryname)
					== B_NAME_TOO_LONG );
	CPPUNIT_ASSERT( link.InitCheck() == B_NAME_TOO_LONG );
	//
	nextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(fileSuperDirname) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(&pathDir, fileRelDirname) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.InitCheck() == B_ENTRY_NOT_FOUND );
}

// ReadLinkTest
void
SymLinkTest::ReadLinkTest()
{
	const char *dirLink = dirLinkname;
	const char *fileLink = fileLinkname;
	const char *badLink = badLinkname;
	const char *cyclicLink1 = cyclicLinkname1;
	const char *cyclicLink2 = cyclicLinkname2;
	const char *existingDir = existingDirname;
	const char *existingFile = existingFilename;
	const char *nonExisting = nonExistingDirname;
	BSymLink link;
	char buffer[B_PATH_NAME_LENGTH + 1];
	// uninitialized
	// R5: returns B_BAD_ADDRESS instead of (as doc'ed) B_FILE_ERROR
	nextSubTest();
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( equals(link.ReadLink(buffer, sizeof(buffer)),
						   B_BAD_ADDRESS, B_FILE_ERROR) );
	// existing dir link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(buffer, sizeof(buffer))
					== (ssize_t)strlen(existingDir) );
	CPPUNIT_ASSERT( strcmp(buffer, existingDir) == 0 );
	// existing file link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(fileLink) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(buffer, sizeof(buffer))
					== (ssize_t)strlen(existingFile) );
	CPPUNIT_ASSERT( strcmp(buffer, existingFile) == 0 );
	// existing cyclic link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(cyclicLink1) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(buffer, sizeof(buffer))
					== (ssize_t)strlen(cyclicLink2) );
	CPPUNIT_ASSERT( strcmp(buffer, cyclicLink2) == 0 );
	// existing link to non-existing entry
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(badLink) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(buffer, sizeof(buffer))
					== (ssize_t)strlen(nonExisting) );
	CPPUNIT_ASSERT( strcmp(buffer, nonExisting) == 0 );
	// non-existing link
	// R5: returns B_BAD_ADDRESS instead of (as doc'ed) B_FILE_ERROR
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( equals(link.ReadLink(buffer, sizeof(buffer)),
						   B_BAD_ADDRESS, B_FILE_ERROR) );
	// dir
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(buffer, sizeof(buffer)) == B_BAD_VALUE );
	// file
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(buffer, sizeof(buffer)) == B_BAD_VALUE );
	// small buffer
	// R5: returns the size of the contents, not the number of bytes copied
	// OBOS: ... so do we
	nextSubTest();
	char smallBuffer[2];
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( link.ReadLink(smallBuffer, sizeof(smallBuffer))
					== (ssize_t)strlen(dirLink) );
	CPPUNIT_ASSERT( strncmp(smallBuffer, existingDir, sizeof(smallBuffer)) == 0 );
	// bad args
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(fileLink) == B_OK );
	CPPUNIT_ASSERT( equals(link.ReadLink(NULL, sizeof(buffer)), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
}

// MakeLinkedPathTest
void
SymLinkTest::MakeLinkedPathTest()
{
	const char *dirLink = dirLinkname;
	const char *fileLink = fileLinkname;
	const char *relDirLink = relDirLinkname;
	const char *relFileLink = relFileLinkname;
	const char *cyclicLink1 = cyclicLinkname1;
	const char *cyclicLink2 = cyclicLinkname2;
	const char *existingDir = existingDirname;
	const char *existingSuperDir = existingSuperDirname;
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *nonExisting = nonExistingDirname;
	BSymLink link;
	BPath path;
	// 1. MakeLinkedPath(const char*, BPath*)
	// uninitialized
	nextSubTest();
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( equals(link.MakeLinkedPath("/boot", &path), B_BAD_ADDRESS,
						   B_FILE_ERROR) );
	link.Unset();
	path.Unset();
	// existing absolute dir link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( link.MakeLinkedPath("/boot", &path)
					== (ssize_t)strlen(existingDir) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(existingDir) == path.Path() );
	link.Unset();
	path.Unset();
	// existing absolute file link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(fileLink) == B_OK );
	CPPUNIT_ASSERT( link.MakeLinkedPath("/boot", &path)
					== (ssize_t)strlen(existingFile) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(existingFile) == path.Path() );
	link.Unset();
	path.Unset();
	// existing absolute cyclic link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(cyclicLink1) == B_OK );
	CPPUNIT_ASSERT( link.MakeLinkedPath("/boot", &path)
					== (ssize_t)strlen(cyclicLink2) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(cyclicLink2) == path.Path() );
	link.Unset();
	path.Unset();
	// existing relative dir link
	nextSubTest();
	BEntry entry;
	BPath entryPath;
	CPPUNIT_ASSERT( entry.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&entryPath) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(relDirLink) == B_OK );
	CPPUNIT_ASSERT( link.MakeLinkedPath(existingSuperDir, &path)
					== (ssize_t)strlen(entryPath.Path()) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entryPath == path );
	link.Unset();
	path.Unset();
	entry.Unset();
	entryPath.Unset();
	// existing relative file link
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&entryPath) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(relFileLink) == B_OK );
	CPPUNIT_ASSERT( link.MakeLinkedPath(existingSuperFile, &path)
					== (ssize_t)strlen(entryPath.Path()) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entryPath == path );
	link.Unset();
	path.Unset();
	entry.Unset();
	entryPath.Unset();
	// bad args
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
// R5: crashs, when passing a NULL path
#if !SK_TEST_R5
	CPPUNIT_ASSERT( link.MakeLinkedPath("/boot", NULL) == B_BAD_VALUE );
#endif
	CPPUNIT_ASSERT( link.MakeLinkedPath((const char*)NULL, &path)
					== B_BAD_VALUE );
// R5: crashs, when passing a NULL path
#if !SK_TEST_R5
	CPPUNIT_ASSERT( link.MakeLinkedPath((const char*)NULL, NULL)
					== B_BAD_VALUE );
#endif
	link.Unset();
	path.Unset();

	// 2. MakeLinkedPath(const BDirectory*, BPath*)
	// uninitialized
	nextSubTest();
	link.Unset();
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	BDirectory dir;
	CPPUNIT_ASSERT( dir.SetTo("/boot") == B_OK);
	CPPUNIT_ASSERT( equals(link.MakeLinkedPath(&dir, &path), B_BAD_ADDRESS,
						   B_FILE_ERROR) );
	link.Unset();
	path.Unset();
	dir.Unset();
	// existing absolute dir link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo("/boot") == B_OK);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(existingDir) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(existingDir) == path.Path() );
	link.Unset();
	path.Unset();
	dir.Unset();
	// existing absolute file link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(fileLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo("/boot") == B_OK);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(existingFile) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(existingFile) == path.Path() );
	link.Unset();
	path.Unset();
	dir.Unset();
	// existing absolute cyclic link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(cyclicLink1) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo("/boot") == B_OK);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(cyclicLink2) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(cyclicLink2) == path.Path() );
	link.Unset();
	path.Unset();
	dir.Unset();
	// existing relative dir link
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&entryPath) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(relDirLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(existingSuperDir) == B_OK);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(entryPath.Path()) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entryPath == path );
	link.Unset();
	path.Unset();
	dir.Unset();
	entry.Unset();
	entryPath.Unset();
	// existing relative file link
	nextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( entry.GetPath(&entryPath) == B_OK );
	CPPUNIT_ASSERT( link.SetTo(relFileLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(existingSuperFile) == B_OK);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(entryPath.Path()) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( entryPath == path );
	link.Unset();
	path.Unset();
	dir.Unset();
	entry.Unset();
	entryPath.Unset();
	// absolute link, uninitialized dir
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(existingDir) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(existingDir) == path.Path() );
	// absolute link, badly initialized dir
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND);
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, &path)
					== (ssize_t)strlen(existingDir) );
	CPPUNIT_ASSERT( path.InitCheck() == B_OK );
	CPPUNIT_ASSERT( string(existingDir) == path.Path() );
	link.Unset();
	path.Unset();
	dir.Unset();
	// relative link, uninitialized dir
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(relDirLink) == B_OK );
	CPPUNIT_ASSERT( dir.InitCheck() == B_NO_INIT);
	CPPUNIT_ASSERT( equals(link.MakeLinkedPath(&dir, &path), B_NO_INIT,
						   B_BAD_VALUE) );
	link.Unset();
	// relative link, badly initialized dir
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(relDirLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo(nonExisting) == B_ENTRY_NOT_FOUND);
	CPPUNIT_ASSERT( equals(link.MakeLinkedPath(&dir, &path), B_NO_INIT,
						   B_BAD_VALUE) );
	link.Unset();
	path.Unset();
	dir.Unset();
	// bad args
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( dir.SetTo("/boot") == B_OK);
// R5: crashs, when passing a NULL path
#if !SK_TEST_R5
	CPPUNIT_ASSERT( link.MakeLinkedPath(&dir, NULL) == B_BAD_VALUE );
#endif

	CPPUNIT_ASSERT( link.MakeLinkedPath((const BDirectory*)NULL, &path)
					== B_BAD_VALUE );
// R5: crashs, when passing a NULL path
#if !SK_TEST_R5
	CPPUNIT_ASSERT( link.MakeLinkedPath((const BDirectory*)NULL, NULL)
					== B_BAD_VALUE );
#endif
	link.Unset();
	path.Unset();
	dir.Unset();
}

// IsAbsoluteTest
void
SymLinkTest::IsAbsoluteTest()
{
	const char *dirLink = dirLinkname;
	const char *relFileLink = relFileLinkname;
	const char *existingDir = existingDirname;
	const char *existingFile = existingFilename;
	const char *nonExisting = nonExistingDirname;
	BSymLink link;
	// uninitialized
	nextSubTest();
	CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( link.IsAbsolute() == false );
	link.Unset();
	// existing absolute dir link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(dirLink) == B_OK );
	CPPUNIT_ASSERT( link.IsAbsolute() == true );
	link.Unset();
	// existing relative file link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(relFileLink) == B_OK );
	CPPUNIT_ASSERT( link.IsAbsolute() == false );
	link.Unset();
	// non-existing link
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( link.IsAbsolute() == false );
	link.Unset();
	// dir
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( link.IsAbsolute() == false );
	link.Unset();
	// file
	nextSubTest();
	CPPUNIT_ASSERT( link.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( link.IsAbsolute() == false );
	link.Unset();
}

// AssignmentTest
void
SymLinkTest::AssignmentTest()
{
	const char *dirLink = dirLinkname;
	const char *fileLink = fileLinkname;
	// 1. copy constructor
	// uninitialized
	nextSubTest();
	{
		BSymLink link;
		CPPUNIT_ASSERT( link.InitCheck() == B_NO_INIT );
		BSymLink link2(link);
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(link2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	// existing dir link
	nextSubTest();
	{
		BSymLink link(dirLink);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
		BSymLink link2(link);
		CPPUNIT_ASSERT( link2.InitCheck() == B_OK );
	}
	// existing file link
	nextSubTest();
	{
		BSymLink link(fileLink);
		CPPUNIT_ASSERT( link.InitCheck() == B_OK );
		BSymLink link2(link);
		CPPUNIT_ASSERT( link2.InitCheck() == B_OK );
	}

	// 2. assignment operator
	// uninitialized
	nextSubTest();
	{
		BSymLink link;
		BSymLink link2;
		link2 = link;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(link2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	nextSubTest();
	{
		BSymLink link;
		BSymLink link2(dirLink);
		link2 = link;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(link2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	// existing dir link
	nextSubTest();
	{
		BSymLink link(dirLink);
		BSymLink link2;
		link2 = link;
		CPPUNIT_ASSERT( link2.InitCheck() == B_OK );
	}
	// existing file link
	nextSubTest();
	{
		BSymLink link(fileLink);
		BSymLink link2;
		link2 = link;
		CPPUNIT_ASSERT( link2.InitCheck() == B_OK );
	}
}


