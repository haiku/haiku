#include "StringInsertTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <iostream>

StringInsertTest::StringInsertTest(std::string name) :
		BTestCase(name)
{
}

 

StringInsertTest::~StringInsertTest()
{
}


void 
StringInsertTest::PerformTest(void)
{
	BString *str1, *str2;
	
	//&Insert(const char *, int32 pos);
	NextSubTest();
	str1 = new BString("String");
	str1->Insert("INSERTED", 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "StrINSERTEDing") == 0);
	delete str1;
	
	//This test crashes both implementations
	//NextSubTest();
	//str1 = new BString("String");
	//str1->Insert("INSERTED", 10);
	//CPPUNIT_ASSERT(strcmp(str1->String(), "string") == 0);
	//delete str1;
	
	NextSubTest();
	str1 = new BString;
	str1->Insert("INSERTED", -1);
	CPPUNIT_ASSERT(strcmp(str1->String(), "NSERTED") == 0);
	delete str1;
	
	//&Insert(const char *, int32 length, int32 pos);
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	delete str1;
	
	//This test crashes both implementations
	//NextSubTest();
	//str1 = new BString("string");
	//str1->Insert("INSERTED", 2, 30);
	//CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	//delete str1;
	
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 10, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINSERTEDring") == 0);
	delete str1;
	
	//&Insert(const char *, int32 fromOffset, int32 length, int32 pos);
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 4, 30, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stRTEDring") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString("string");
	str1->Insert('P', 5, 3);
	cerr << str1->String() << endl;
	CPPUNIT_ASSERT(strcmp(str1->String(), "strPPPPPing") == 0);
	delete str1;
}


CppUnit::Test *StringInsertTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringInsertTest>
		StringInsertTestCaller;
		
	return(new StringInsertTestCaller("BString::Insert Test", &StringInsertTest::PerformTest));
}
