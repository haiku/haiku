#include "StringRemoveTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <stdio.h>

StringRemoveTest::StringRemoveTest(std::string name) :
		BTestCase(name)
{
}

 

StringRemoveTest::~StringRemoveTest()
{
}


void 
StringRemoveTest::PerformTest(void)
{
	BString *string1, *string2;
	
	//Truncate(int32 newLength, bool lazy);
	//lazy = true
	NextSubTest();
	string1 = new BString("This is a long string");
	string1->Truncate(14, true);
	CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long") == 0);
	CPPUNIT_ASSERT(string1->Length() == 14);
	delete string1;
	
	//lazy = false
	NextSubTest();
	string1 = new BString("This is a long string");
	string1->Truncate(14, false);
	CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long") == 0);
	CPPUNIT_ASSERT(string1->Length() == 14);
	delete string1;
	
	//new length is < 0
	//commented out as it crashes r5 implementation
	//NextSubTest();
	//string1 = new BString("This is a long string");
	//string1->Truncate(-3);
	//CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long string") == 0);
	//CPPUNIT_ASSERT(string1->Length() == 21);
	//delete string1;	
	
	//new length is > old length
	NextSubTest();
	string1 = new BString("This is a long string");
	string1->Truncate(45);
	CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long string") == 0);
	CPPUNIT_ASSERT(string1->Length() == 21);
	delete string1;
	
	//String was empty
	NextSubTest();
	string1 = new BString;
	string1->Truncate(0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	CPPUNIT_ASSERT(string1->Length() == 0);
	delete string1;
	
	//Remove(int32 from, int32 length)
	NextSubTest();
	string1 = new BString("a String");
	string1->Remove(2, 2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a ring") == 0);
	delete string1;
	
	//String was empty
	NextSubTest();
	string1 = new BString;
	string1->Remove(2, 1);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
	
	//from is beyond the end of the string
	NextSubTest();
	string1 = new BString("a String");
	string1->Remove(20, 2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a String") == 0);
	delete string1;
	
	//from + length is > Length()
	NextSubTest();
	string1 = new BString("a String");
	string1->Remove(4, 30);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a String") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("a String");
	string1->Remove(-3, 5);
	CPPUNIT_ASSERT(strcmp(string1->String(), "ing") == 0);
	delete string1;
}


CppUnit::Test *StringRemoveTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringRemoveTest>
		StringRemoveTestCaller;
		
	return(new StringRemoveTestCaller("BString::Remove Test", &StringRemoveTest::PerformTest));
}
