// VolumeTest.h

#ifndef VOLUME_TEST_H
#define VOLUME_TEST_H

#include "BasicTest.h"

#include <StorageDefs.h>
#include <SupportDefs.h>

class BTestApp;
class BBitmap;
class CppUnit::Test;
class TestHandler;

class VolumeTest : public BasicTest
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
	void AssignmentTest();
	void ComparissonTest();
	void SetNameTest();
	void BadValuesTest();

	void IterationTest();
	void WatchingTest();


private:
	BTestApp	*fApplication;
};

#endif	// VOLUME_TEST_H
