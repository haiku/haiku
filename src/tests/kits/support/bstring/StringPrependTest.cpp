#include "StringPrependTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <UTF8.h>

StringPrependTest::StringPrependTest(std::string name) :
		BTestCase(name)
{
}

 

StringPrependTest::~StringPrependTest()
{
}


void 
StringPrependTest::PerformTest(void)
{
	BString *str1, *str2;
	
	//Prepend(BString&)
	NextSubTest();
	str1 = new BString("a String");
	str2 = new BString("PREPENDED");
	str1->Prepend(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPENDEDa String") == 0);
	delete str1;
	delete str2;
	
	//Prepend(const char*)
	NextSubTest();
	str1 = new BString("String");
	str1->Prepend("PREPEND");
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPENDString") == 0);
	delete str1;
	
	//Prepend(const char*) (NULL)
	NextSubTest();
	str1 = new BString("String");
	str1->Prepend((char*)NULL);
	CPPUNIT_ASSERT(strcmp(str1->String(), "String") == 0);
	delete str1;
	
	//Prepend(const char*, int32
	NextSubTest();
	str1 = new BString("String");
	str1->Prepend("PREPENDED", 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREString") == 0);
	delete str1;
	
	//Prepend(BString&, int32)
	NextSubTest();
	str1 = new BString("String");
	str2 = new BString("PREPEND", 4);
	str1->Prepend(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPString") == 0);
	delete str1;
	delete str2;
	
	//Prepend(char, int32)
	NextSubTest();
	str1 = new BString("aString");
	str1->Prepend('c', 4);
	CPPUNIT_ASSERT(strcmp(str1->String(), "ccccaString") == 0);
	delete str1;
	
	//String was empty
	NextSubTest();
	str1 = new BString;
	str1->Prepend("PREPENDED");
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPENDED") == 0);
	delete str1;

}


CppUnit::Test *StringPrependTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringPrependTest>
		StringPrependTestCaller;
		
	return(new StringPrependTestCaller("BString::Prepend Test", &StringPrependTest::PerformTest));
}
