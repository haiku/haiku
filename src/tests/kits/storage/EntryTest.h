// EntryTest.h

#ifndef __sk_entry_test_h__
#define __sk_entry_test_h__

#include <TestUtils.h>
#include "StatableTest.h"

class BEntry;
struct TestEntry;

class EntryTest : public StatableTest
{
public:
	static CppUnit::Test* Suite();

	virtual void CreateROStatables(TestStatables& testEntries);
	virtual void CreateRWStatables(TestStatables& testEntries);
	virtual void CreateUninitializedStatables(TestStatables& testEntries);

	// This function called before *each* test added in Suite()
	void setUp();

	// This function called after *each* test added in Suite()
	void tearDown();

	//-------------------------------------------------------------------------
	// Test functions
	//-------------------------------------------------------------------------
	void InitTest1Paths(TestEntry &testEntry, status_t error,
						bool traverse = false);
	void InitTest1Refs(TestEntry &testEntry, status_t error,
					   bool traverse = false);
	void InitTest1DirPaths(TestEntry &testEntry, status_t error,
						   bool traverse = false);
	void InitTest1();

	void InitTest2Paths(TestEntry &testEntry, status_t error,
						bool traverse = false);
	void InitTest2Refs(TestEntry &testEntry, status_t error,
					   bool traverse = false);
	void InitTest2DirPaths(TestEntry &testEntry, status_t error,
						   bool traverse = false);
	void InitTest2();

	void ExistenceTest();
	void SpecialGetCasesTest();

	void RenameTestEntry(TestEntry *testEntry, TestEntry *newTestEntry,
						 string newName, bool existing, bool clobber,
						 status_t error, uint32 kind);
	void RenameTestEntry(TestEntry *testEntry, TestEntry *newTestEntry,
						 bool existing, bool clobber,
						 status_t error, uint32 kind);
	void RenameTestFile(TestEntry *testEntry, TestEntry *newTestEntry,
						bool existing, bool clobber, status_t error);
	void RenameTestDir(TestEntry *testEntry, TestEntry *newTestEntry,
					   bool existing, bool clobber, status_t error);
	void RenameTestLink(TestEntry *testEntry, TestEntry *newTestEntry,
						bool existing, bool clobber, status_t error);
	void RenameTest();

	void MoveToTestEntry(TestEntry *testEntry, TestEntry *testDir,
						 string newName, bool existing, bool clobber,
						 status_t error, uint32 kind);
	void MoveToTestEntry(TestEntry *testEntry, TestEntry *testDir,
						 TestEntry *newTestEntry, bool existing, bool clobber,
						 status_t error, uint32 kind);
	void MoveToTestFile(TestEntry *testEntry, TestEntry *testDir,
						TestEntry *newTestEntry, bool existing, bool clobber,
						status_t error);
	void MoveToTestDir(TestEntry *testEntry, TestEntry *testDir,
					   TestEntry *newTestEntry, bool existing, bool clobber,
					   status_t error);
	void MoveToTestLink(TestEntry *testEntry, TestEntry *testDir,
						TestEntry *newTestEntry, bool existing, bool clobber,
						status_t error);
	void MoveToTest();

	void RemoveTest();
	void ComparisonTest();
	void AssignmentTest();	
	void MiscTest();
	void CFunctionsTest();

	//-------------------------------------------------------------------------
	// Helper functions
	//-------------------------------------------------------------------------	
	// Creates a symbolic link named link that points to target
	void CreateLink(const char *link, const char *target);
	// Creates the given file
	void CreateFile(const char *file);
	// Creates the given file
	void CreateDir(const char *dir);
	// Removes the given file if it exists
	void RemoveFile(const char *file);
	// Checks, whether a file exists.
	bool PingFile(const char *path, BEntry *entry = NULL);
	// Checks, whether a dir exists.
	bool PingDir(const char *path, BEntry *entry = NULL);
	// Checks, whether a symlink exists.
	bool PingLink(const char *path, const char *target = NULL,
				  BEntry *entry = NULL);
};


#endif	// __sk_entry_test_h__
