#include "StringEscapeTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringEscapeTest::StringEscapeTest(std::string name) :
		BTestCase(name)
{
}

 

StringEscapeTest::~StringEscapeTest()
{
}


void 
StringEscapeTest::PerformTest(void)
{
	BString *string1, *string2;
	
	NextSubTest();
	string1 = new BString("abcdefghi");
	string1->CharacterEscape("acf", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/ab/cde/fghi") == 0);
	delete string1;			
}


CppUnit::Test *StringEscapeTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringEscapeTest>
		StringEscapeTestCaller;
		
	return(new StringEscapeTestCaller("BString::Escape Test", &StringEscapeTest::PerformTest));
}
