// SymLinkTest.h

#ifndef __sk_sym_link_test_h__
#define __sk_sym_link_test_h__

#include <SupportDefs.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "NodeTest.h"

class SymLinkTest : public NodeTest
{
public:
	static Test* Suite();

	virtual void CreateRONodes(TestNodes& testEntries);
	virtual void CreateRWNodes(TestNodes& testEntries);
	virtual void CreateUninitializedNodes(TestNodes& testEntries);

	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	// test methods

	void InitTest1();
	void InitTest2();
	void ReadLinkTest();
	void MakeLinkedPathTest();
	void IsAbsoluteTest();
	void AssignmentTest();
};

#endif	// __sk_sym_link_test_h__




