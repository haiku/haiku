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
	CPPUNIT_ASSERT((unsigned)string.Length() == strlen(string.String()));

	NextSubTest();
	BString string2("ABCD");
	CPPUNIT_ASSERT(string2.CountChars() == 4);
	CPPUNIT_ASSERT((unsigned)string2.Length() == strlen(string2.String()));

	NextSubTest();
	static char s[64];
	strcpy(s, B_UTF8_ELLIPSIS);
	strcat(s, B_UTF8_SMILING_FACE);
	BString string3(s);
	CPPUNIT_ASSERT(string3.CountChars() == 2);	
	CPPUNIT_ASSERT((unsigned)string3.Length() == strlen(string3.String()));
	
	//An empty string
	NextSubTest();
	BString empty;
	CPPUNIT_ASSERT(strcmp(empty.String(), "") == 0);
	CPPUNIT_ASSERT(empty.Length() == 0);
	CPPUNIT_ASSERT(empty.CountChars() == 0);

	//Truncate the string at end so we are left with an invalid
	//UTF8 character
	NextSubTest();
	BString invalid("some text with utf8 characters"B_UTF8_ELLIPSIS);
	invalid.Truncate(invalid.Length() -1);
	CPPUNIT_ASSERT(invalid.CountChars() == 31);

	//LockBuffer(int32) and UnlockBuffer(int32)
	NextSubTest();
	BString locked("a string");
	char *ptrstr = locked.LockBuffer(20);
	CPPUNIT_ASSERT(strcmp(ptrstr, "a string") == 0);
	strcat(ptrstr, " to be locked");
	locked.UnlockBuffer();
	CPPUNIT_ASSERT(strcmp(ptrstr, "a string to be locked") == 0);
	
	NextSubTest();
	BString locked2("some text");
	char *ptr = locked2.LockBuffer(3);
	CPPUNIT_ASSERT(strcmp(ptr, "some text") == 0);
	locked2.UnlockBuffer(4);
	CPPUNIT_ASSERT(strcmp(locked2.String(), "some") == 0);
	CPPUNIT_ASSERT(locked2.Length() == 4);
	
	NextSubTest();
	BString emptylocked;
	ptr = emptylocked.LockBuffer(10);
	CPPUNIT_ASSERT(strcmp(ptr, "") == 0);
	strcat(ptr, "pippo");
	emptylocked.UnlockBuffer();
	CPPUNIT_ASSERT(strcmp(emptylocked.String(), "pippo") == 0);
	
	// LockBuffer(0) and UnlockBuffer(-1) on a zero lenght string
#ifndef TEST_R5	
	NextSubTest();
	BString crashesR5;
	ptr = crashesR5.LockBuffer(0);
	crashesR5.UnlockBuffer(-1);
	CPPUNIT_ASSERT(strcmp(crashesR5.String(), "") == 0);
#endif
}


CppUnit::Test *StringAccessTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringAccessTest>
		StringAccessTestCaller;
		
	return(new StringAccessTestCaller("BString::Access Test", &StringAccessTest::PerformTest));
}
