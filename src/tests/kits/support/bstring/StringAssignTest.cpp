#include "StringAssignTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringAssignTest::StringAssignTest(std::string name) :
		BTestCase(name)
{
}

 

StringAssignTest::~StringAssignTest()
{
}


void 
StringAssignTest::PerformTest(void)
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
	str = new BString;
	str->SetTo(s);
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;
	
	NextSubTest();
	str = new BString;
	str->SetTo("BLA");
	CPPUNIT_ASSERT(strcmp(str->String(), "BLA") == 0);
	delete str;
	
	NextSubTest();
	str = new BString;
	str->SetTo(string);
	CPPUNIT_ASSERT(strcmp(str->String(), string.String()) == 0);
	delete str;
	
	NextSubTest();
	str = new BString;
	str->SetTo('C', 10);
	CPPUNIT_ASSERT(strcmp(str->String(), "CCCCCCCCCC") == 0);
	delete str;
	
	NextSubTest();
	str = new BString("ASDSGAFA");
	str->SetTo('C', 0);
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;
	
	NextSubTest();
	str = new BString;
	str->SetTo("ABC", 10);
	CPPUNIT_ASSERT(strcmp(str->String(), "ABC") == 0);
	delete str;
	
	NextSubTest();
	const char *oldString2 = string2.String();
	str = new BString;
	str->Adopt(string2);
	CPPUNIT_ASSERT(strcmp(str->String(), oldString2) == 0);
	CPPUNIT_ASSERT(strcmp(string2.String(), "") == 0);
	delete str;
	
	NextSubTest();
	BString newstring("SomethingElseAgain");
	const char *oldString = newstring.String();
	str = new BString;
	str->Adopt(newstring, 2);
	CPPUNIT_ASSERT(strncmp(str->String(), oldString, 2) == 0);
	CPPUNIT_ASSERT(str->Length() == 2);
	CPPUNIT_ASSERT(strcmp(newstring.String(), "") == 0);
	delete str;
}


CppUnit::Test *StringAssignTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringAssignTest>
		StringAssignTestCaller;
		
	return(new StringAssignTestCaller("BString::Assign Test", &StringAssignTest::PerformTest));
}
