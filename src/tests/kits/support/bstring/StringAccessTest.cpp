#include "StringAccessTest.h"
#include "cppunit/TestCaller.h"
#include <stdio.h>
#include <String.h>
#include <UTF8.h>

StringAccessTest::StringAccessTest(std::string name) :
		BTestCase(name)
{
}

 

StringAccessTest::~StringAccessTest()
{
}


void 
StringAccessTest::PerformTest(void)
{
	//CountChars(), Length(), String()
	NextSubTest();
	BString string("Something"B_UTF8_ELLIPSIS);
	CPPUNIT_ASSERT(string.CountChars() == 10);
	CPPUNIT_ASSERT(string.Length() == strlen(string.String()));

	NextSubTest();
	BString string2("ABCD");
	CPPUNIT_ASSERT(string2.CountChars() == 4);
	CPPUNIT_ASSERT(string2.Length() == strlen(string2.String()));

	NextSubTest();
	static char s[64];
	strcpy(s, B_UTF8_ELLIPSIS);
	strcat(s, B_UTF8_SMILING_FACE);
	BString string3(s);
	CPPUNIT_ASSERT(string3.CountChars() == 2);	
	CPPUNIT_ASSERT(string3.Length() == strlen(string3.String()));

	NextSubTest();
	BString empty;
	CPPUNIT_ASSERT(strcmp(empty.String(), "") == 0);
	CPPUNIT_ASSERT(empty.Length() == 0);
	CPPUNIT_ASSERT(empty.CountChars() == 0);

}


CppUnit::Test *StringAccessTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringAccessTest>
		StringAccessTestCaller;
		
	return(new StringAccessTestCaller("BString::Access Test", &StringAccessTest::PerformTest));
}
