#include "StringReplaceTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringReplaceTest::StringReplaceTest(std::string name) :
		BTestCase(name)
{
}

 

StringReplaceTest::~StringReplaceTest()
{
}


void 
StringReplaceTest::PerformTest(void)
{
	BString *str1, *str2;
	
	//&ReplaceFirst(char, char);
	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceFirst('t', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "best string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceFirst('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&ReplaceLast(char, char);
	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceLast('t', 'w');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test swring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceLast('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&ReplaceAll(char, char, int32);
	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('t', 'i');
	CPPUNIT_ASSERT(strcmp(str1->String(), "iesi siring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('t', 'i', 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "tesi siring") == 0);
	delete str1;

	//&Replace(char, char, int32, int32)
	NextSubTest();
	str1 = new BString("she sells sea shells on the sea shore");
	str1->Replace('s' 't', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "she tellt tea thells on the sea shore") == 0);
	delete str1;
}


CppUnit::Test *StringReplaceTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringReplaceTest>
		StringReplaceTestCaller;
		
	return(new StringReplaceTestCaller("BString::Replace Test", &StringReplaceTest::PerformTest));
}
