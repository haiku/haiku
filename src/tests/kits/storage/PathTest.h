// PathTest.h

#ifndef __sk_path_test_h__
#define __sk_path_test_h__

#include "BasicTest.h"

#include <StorageDefs.h>
#include <SupportDefs.h>

class PathTest : public BasicTest
{
public:
	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void InitTest1();
	void InitTest2();
	void AppendTest();
	void LeafTest();
	void ParentTest();
	void ComparisonTest();
	void AssignmentTest();
	void FlattenableTest();
};

#endif	// __sk_path_test_h__
