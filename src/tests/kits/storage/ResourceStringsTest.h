// ResourceStringsTest.h

#ifndef __sk_resource_strings_test_h__
#define __sk_resource_strings_test_h__

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <StorageDefs.h>
#include <SupportDefs.h>

#include "BasicTest.h"

class ResourceStringsTest : public BasicTest
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
	void FindStringTest();

//	BResourceStrings();
//	BResourceStrings(const entry_ref &ref);
//	virtual ~BResourceStrings();
//	virtual status_t SetStringFile(const entry_ref *ref);
//	status_t GetStringFile(entry_ref *outRef);
//	status_t InitCheck();

//	virtual BString *NewString(int32 id);
//	virtual const char *FindString(int32 id);



};


#endif	// __sk_resource_strings_test_h__




