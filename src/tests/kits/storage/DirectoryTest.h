// DirectoryTest.h

#ifndef __sk_directory_test_h__
#define __sk_directory_test_h__

#include <SupportDefs.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "NodeTest.h"

class DirectoryTest : public NodeTest
{
public:
	static Test* Suite();

	virtual void CreateRONodes(TestNodes& testEntries);
	virtual void CreateRWNodes(TestNodes& testEntries);
	virtual void CreateUninitializedNodes(TestNodes& testEntries);

	// This function is called before *each* test added in Suite()
	void setUp();
	
	// This function is called after *each* test added in Suite()
	void tearDown();

	// test methods

	void InitTest1();
	void InitTest2();
	void GetEntryTest();
	void IsRootTest();
	void FindEntryTest();
	void ContainsTest();
	void GetStatForTest();
	void EntryIterationTest();
	void EntryCreationTest();
	void AssignmentTest();
	void CreateDirectoryTest();
};

#endif	// __sk_directory_test_h__
