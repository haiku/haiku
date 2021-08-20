#include "StringInsertTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringInsertTest::StringInsertTest(std::string name)
		: BTestCase(name)
{
}


StringInsertTest::~StringInsertTest()
{
}


void 
StringInsertTest::PerformTest(void)
{
	BString *str1, *str2;
	
	// &Insert(const char *, int32 pos);
	NextSubTest();
	str1 = new BString("String");
	str1->Insert("INSERTED", 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "StrINSERTEDing") == 0);
	delete str1;
	
#ifndef TEST_R5
	// This test crashes R5 and should drop into the debugger in Haiku
	// (if compiled with DEBUG):
	NextSubTest();
	str1 = new BString("String");
	str1->Insert("INSERTED", 10);
	CPPUNIT_ASSERT(strcmp(str1->String(), "String") == 0);
	delete str1;
#endif
	
	NextSubTest();
	str1 = new BString;
	str1->Insert("INSERTED", -1);
	CPPUNIT_ASSERT(strcmp(str1->String(), "NSERTED") == 0);
	delete str1;
	
#ifndef TEST_R5
	// check limitation of negative values (R5 doesn't):
	NextSubTest();
	str1 = new BString;
	str1->Insert("INSERTED", -142364253);
	CPPUNIT_ASSERT(strcmp(str1->String(), "") == 0);
	delete str1;
#endif
	
	// &Insert(const char *, int32 length, int32 pos);
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	delete str1;
	
#ifndef TEST_R5
	// This test crashes R5 and should drop into the debugger in Haiku
	// (if compiled with DEBUG):
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 2, 30);
	CPPUNIT_ASSERT(strcmp(str1->String(), "string") == 0);
	delete str1;
#endif
	
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 10, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINSERTEDring") == 0);
	delete str1;
	
	// &Insert(const char *, int32 fromOffset, int32 length, int32 pos);
	NextSubTest();
	str1 = new BString("string");
	str1->Insert("INSERTED", 4, 30, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stRTEDring") == 0);
	delete str1;
	
	// Insert(char c, int32 count, int32 pos)
	NextSubTest();
	str1 = new BString("string");
	str1->Insert('P', 5, 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "strPPPPPing") == 0);
	delete str1;
	
	// Insert(char c, int32 count, int32 pos)
	NextSubTest();
	str1 = new BString("string");
	str1->Insert('P', 5, -2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PPPstring") == 0);
	delete str1;
	
	// Insert(BString&)
	NextSubTest();
	str1 = new BString("string");
	str2 = new BString("INSERTED");
	str1->Insert(*str2, 0);
	CPPUNIT_ASSERT(strcmp(str1->String(), "INSERTEDstring") == 0);
	delete str1;
	delete str2;
	
	NextSubTest();
	str1 = new BString("string");
	str1->Insert(*str1, 0);
	CPPUNIT_ASSERT(strcmp(str1->String(), "string") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString;
	str2 = new BString("INSERTED");
	str1->Insert(*str2, -1);
	CPPUNIT_ASSERT(strcmp(str1->String(), "NSERTED") == 0);
	delete str1;
	delete str2;
	
	// &Insert(BString &, int32 length, int32 pos);
	NextSubTest();
	str1 = new BString("string");
	str2 = new BString("INSERTED");
	str1->Insert(*str2, 2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	delete str1;
	delete str2;
		
	// &Insert(BString&, int32 fromOffset, int32 length, int32 pos);
	NextSubTest();
	str1 = new BString("string");
	str2 = new BString("INSERTED");
	str1->Insert(*str2, 4, 30, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stRTEDring") == 0);
	delete str1;
	delete str2;
}


CppUnit::Test *StringInsertTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringInsertTest>
		StringInsertTestCaller;
		
	return(new StringInsertTestCaller("BString::Insert Test",
		&StringInsertTest::PerformTest));
}
