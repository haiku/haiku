#include "StringOperatorsTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringOperatorsTest::StringOperatorsTest(std::string name) :
		BTestCase(name)
{
}

 

StringOperatorsTest::~StringOperatorsTest()
{
}


void 
StringOperatorsTest::PerformTest(void)
{
	NextSubTest();
	BString string;
	BString string2("Something");
	string = string2;
	CPPUNIT_ASSERT(strcmp(string.String(), string2.String()) == 0);
	CPPUNIT_ASSERT(strcmp(string.String(), "Something") == 0);
	
	NextSubTest();
	BString *str = new BString();
	*str = "Something Else";
	CPPUNIT_ASSERT(strcmp(str->String(), "Something Else") == 0);
	delete str;
	
	NextSubTest();
	char *s = NULL;
	str = new BString;
	*str = s;
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;
	
	NextSubTest();
	str = new BString("WaitFor");
	*str += string2;
	CPPUNIT_ASSERT(strcmp(str->String(), "WaitForSomething") == 0);
	delete str;
	
	NextSubTest();
	str = new BString("Some");
	*str += "thing";
	CPPUNIT_ASSERT(strcmp(str->String(), "Something") == 0);
	delete str;
	
	NextSubTest();
	str = new BString("Some");
	*str += (char*)NULL;
	CPPUNIT_ASSERT(strcmp(str->String(), "Some") == 0);
	delete str;
	
	NextSubTest();
	str = new BString("Somethin");
	*str += 'g';
	CPPUNIT_ASSERT(strcmp(str->String(), "Something") == 0);
	delete str;	
}


CppUnit::Test *StringOperatorsTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringOperatorsTest>
		StringOperatorsTestCaller;
		
	return(new StringOperatorsTestCaller("BString::Operators Test", &StringOperatorsTest::PerformTest));
}
