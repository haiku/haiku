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

#ifndef TEST_R5	
	//new length is < 0
	//it crashes r5 implementation, but ours works fine here,
	//in this case, we just truncate to 0
	NextSubTest();
	string1 = new BString("This is a long string");
	string1->Truncate(-3);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	CPPUNIT_ASSERT(string1->Length() == 0);
	delete string1;	
#endif
	
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
	
	//from + length exceeds Length() (R5 fails)
	NextSubTest();
	string1 = new BString("a String");
	string1->Remove(4, 30);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a St") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("a String");
	string1->Remove(-3, 5);
	CPPUNIT_ASSERT(strcmp(string1->String(), "ing") == 0);
	delete string1;
	
	//RemoveFirst(BString&)
	NextSubTest();
	string1 = new BString("first second first");
	string2 = new BString("first");
	string1->RemoveFirst(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), " second first") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("first second first");
	string2 = new BString("noway");
	string1->RemoveFirst(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	delete string2;
	
	//RemoveLast(Bstring&)
	NextSubTest();
	string1 = new BString("first second first");
	string2 = new BString("first");
	string1->RemoveLast(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second ") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("first second first");
	string2 = new BString("noway");
	string1->RemoveLast(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	delete string2;
	
	//RemoveAll(BString&)
	NextSubTest();
	string1 = new BString("first second first");
	string2 = new BString("first");
	string1->RemoveAll(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), " second ") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("first second first");
	string2 = new BString("noway");
	string1->RemoveAll(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	delete string2;
	
	//RemoveFirst(const char*)
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveFirst("first");
	CPPUNIT_ASSERT(strcmp(string1->String(), " second first") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveFirst("noway");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveFirst((char*)NULL);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	//RemoveLast(const char*)
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveLast("first");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second ") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveLast("noway");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	//RemoveAll(const char*)
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveAll("first");
	CPPUNIT_ASSERT(strcmp(string1->String(), " second ") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("first second first");
	string1->RemoveAll("noway");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	//RemoveSet(const char*)
	NextSubTest();
	string1 = new BString("a sentence with (3) (642) numbers (2) in it");
	string1->RemoveSet("()3624 ");
	CPPUNIT_ASSERT(strcmp(string1->String(), "asentencewithnumbersinit") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("a string");
	string1->RemoveSet("1345");
	CPPUNIT_ASSERT(strcmp(string1->String(), "a string") == 0);
	delete string1;
	
	//MoveInto(BString &into, int32, int32)
	NextSubTest();
	string1 = new BString("some text");
	string2 = new BString("string");
	string2->MoveInto(*string1, 3, 2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "in") == 0);
	CPPUNIT_ASSERT(strcmp(string2->String(), "strg") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("some text");
	string2 = new BString("string");
	string2->MoveInto(*string1, 0, 200);
	CPPUNIT_ASSERT(strcmp(string1->String(), "string") == 0);
	CPPUNIT_ASSERT(strcmp(string2->String(), "") == 0);
	delete string1;
	delete string2;
	
	//MoveInto(char *, int32, int32)
	NextSubTest();
	char dest[100];
	memset(dest, 0, 100);
	string1 = new BString("some text");
	string1->MoveInto(dest, 3, 2);
	CPPUNIT_ASSERT(strcmp(dest, "e ") == 0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "somtext") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BString("some text");
	memset(dest, 0, 100);
	string1->MoveInto(dest, 0, 50);
	CPPUNIT_ASSERT(strcmp(dest, "some text") == 0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
}


CppUnit::Test *StringRemoveTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringRemoveTest>
		StringRemoveTestCaller;
		
	return(new StringRemoveTestCaller("BString::Remove Test", &StringRemoveTest::PerformTest));
}
