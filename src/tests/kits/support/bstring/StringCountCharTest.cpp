#include "StringCountCharTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <UTF8.h>

StringCountCharTest::StringCountCharTest(std::string name) :
		BTestCase(name)
{
}

 

StringCountCharTest::~StringCountCharTest()
{
}


void 
StringCountCharTest::PerformTest(void)
{
	//CountChars()
	NextSubTest();
	BString string("Something"B_UTF8_ELLIPSIS);
	CPPUNIT_ASSERT(string.CountChars() == 10);
	
	NextSubTest();
	BString string2("ABCD");
	CPPUNIT_ASSERT(string2.CountChars() == 4);
	
	NextSubTest();
	static char s[64];
	strcpy(s, B_UTF8_ELLIPSIS);
	strcat(s, B_UTF8_SMILING_FACE);
	BString string3(s);
	CPPUNIT_ASSERT(string3.CountChars() == 2);	
}


CppUnit::Test *StringCountCharTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringCountCharTest>
		StringCountCharTestCaller;
		
	return(new StringCountCharTestCaller("BString::CountChar Test", &StringCountCharTest::PerformTest));
}
