#include "StringConstructionTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringConstructionTest::StringConstructionTest(std::string name) :
		BTestCase(name)
{
}

 

StringConstructionTest::~StringConstructionTest()
{
}


void 
StringConstructionTest::PerformTest(void)
{
	BString *string;
	const char *str = "Something";
	
	NextSubTest();
	string = new BString;
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	CPPUNIT_ASSERT(string->Length() == 0);
	delete string;
	
	NextSubTest();
	string = new BString(str);
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT(string->Length() == strlen(str));
	delete string;
	
	NextSubTest();
	string = new BString(NULL);
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	CPPUNIT_ASSERT(string->Length() == 0);
	delete string;
	
	NextSubTest();
	BString anotherString("Something Else");
	string = new BString(anotherString);
	CPPUNIT_ASSERT(strcmp(string->String(), anotherString.String()) == 0);
	CPPUNIT_ASSERT(string->Length() == anotherString.Length());
	delete string;
	
	NextSubTest();
	string = new BString(str, 5);
	CPPUNIT_ASSERT(strcmp(string->String(), str) != 0);
	CPPUNIT_ASSERT(strncmp(string->String(), str, 5) == 0);
	CPPUNIT_ASSERT(string->Length() == 5);
	delete string;
	
	NextSubTest();
	string = new BString(str, 255);
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT(string->Length() == strlen(str));
	delete string;	
}


CppUnit::Test *StringConstructionTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringConstructionTest>
		StringConstructionTestCaller;
		
	return(new StringConstructionTestCaller("BString::Construction Test", &StringConstructionTest::PerformTest));
}
