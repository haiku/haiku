// VolumeTest.h

#ifndef VOLUME_TEST_H
#define VOLUME_TEST_H

#include <StorageDefs.h>
#include <SupportDefs.h>
#include <BasicTest.h>

class BApplication;
class BBitmap;
class CppUnit::Test;

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
	void Assignment();
	void Comparisson();
	void SetName();

private:
	BApplication	*fApplication;
};

#endif	// VOLUME_TEST_H
