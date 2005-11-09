// AppFileInfoTest.h

#ifndef APP_FILE_INFO_TEST_H
#define APP_FILE_INFO_TEST_H

#include "BasicTest.h"

#include <StorageDefs.h>
#include <SupportDefs.h>

class BApplication;
class BBitmap;
class CppUnit::Test;

class AppFileInfoTest : public BasicTest
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
	void TypeTest();
	void SignatureTest();
	void AppFlagsTest();
	void SupportedTypesTest();
	void IconTest();
	void VersionInfoTest();
	void IconForTypeTest();
	void InfoLocationTest();

private:
	BApplication	*fApplication;
	BBitmap			*fIconM1;
	BBitmap			*fIconM2;
	BBitmap			*fIconM3;
	BBitmap			*fIconM4;
	BBitmap			*fIconL1;
	BBitmap			*fIconL2;
	BBitmap			*fIconL3;
	BBitmap			*fIconL4;
};

#endif	// APP_FILE_INFO_TEST_H
