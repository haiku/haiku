#include "StringSubCopyTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <stdio.h>

StringSubCopyTest::StringSubCopyTest(std::string name) :
		BTestCase(name)
{
}

 

StringSubCopyTest::~StringSubCopyTest()
{
}


void 
StringSubCopyTest::PerformTest(void)
{
	BString *string1, *string2;
	
	//CopyInto(BString&, int32, int32)
	NextSubTest();
	string1 = new BString;
	string2 = new BString("Something");
	string2->CopyInto(*string1, 4, 30);
	CPPUNIT_ASSERT(strcmp(string1->String(), "thing") == 0);
	delete string1;
	delete string2;
	
	//CopyInto(const char*, int32, int32)
	NextSubTest();
	char tmp[10];
	memset(tmp, 0, 10);
	string1 = new BString("ABC");
	string1->CopyInto(tmp, 0, 4);
	CPPUNIT_ASSERT(strcmp(tmp, "ABC") == 0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "ABC") == 0);
	delete string1;			
}


CppUnit::Test *StringSubCopyTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringSubCopyTest>
		StringSubCopyTestCaller;
		
	return(new StringSubCopyTestCaller("BString::SubCopy Test", &StringSubCopyTest::PerformTest));
}
