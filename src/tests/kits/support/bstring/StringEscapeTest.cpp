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
	BString *string1;
	
	//CharacterEscape(char*, char)
	NextSubTest();
	string1 = new BString("abcdefghi");
	string1->CharacterEscape("acf", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/ab/cde/fghi") == 0);
	delete string1;
	
	//BString is null
	NextSubTest();
	string1 = new BString;
	string1->CharacterEscape("abc", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
	
	//BString doesn't contain wanted characters
	NextSubTest();
	string1 = new BString("abcdefghi");
	string1->CharacterEscape("z34", 'z');
	CPPUNIT_ASSERT(strcmp(string1->String(), "abcdefghi") == 0);
	delete string1;
	
	//CharacterEscape(char *, char*, char)
	NextSubTest();
	string1 = new BString("something");
	string1->CharacterEscape("newstring", "esi", '0');
	CPPUNIT_ASSERT(strcmp(string1->String(), "n0ew0str0ing") == 0);
	delete string1;

#ifndef TEST_R5	
	//assigned string is NULL
	//it crashes r5 implementation, but not ours :)
	NextSubTest();
	string1 = new BString("something");
	string1->CharacterEscape((char*)NULL, "ei", '-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
#endif
	
	//String was empty
	NextSubTest();
	string1 = new BString;
	string1->CharacterEscape("newstring", "esi", '0');
	CPPUNIT_ASSERT(strcmp(string1->String(), "n0ew0str0ing") == 0);
	delete string1;
	
	//CharacterDeescape(char)
	NextSubTest();
	string1 = new BString("/a/nh/g/bhhgy/fgtuhjkb/");
	string1->CharacterDeescape('/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "anhgbhhgyfgtuhjkb") == 0);
	delete string1;
	
	//String was empty
	NextSubTest();
	string1 = new BString;
	string1->CharacterDeescape('/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;	
	
	//String doesn't contain character to escape
	NextSubTest();
	string1 = new BString("/a/nh/g/bhhgy/fgtuhjkb/");
	string1->CharacterDeescape('-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/a/nh/g/bhhgy/fgtuhjkb/") == 0);
	delete string1;
	
	//CharacterDeescape(char* original, char)
	NextSubTest();
	string1 = new BString("oldString");
	string1->CharacterDeescape("-ne-ws-tri-ng-", '-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "newstring") == 0);
	delete string1;	
	
	//String was empty
	NextSubTest();
	string1 = new BString;
	string1->CharacterDeescape("new/str/ing", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "newstring") == 0);
	delete string1;	

#ifndef TEST_R5	
	//assigned string is empty
	//it crashes r5 implementation, but not ours :)
	NextSubTest();
	string1 = new BString("pippo");
	string1->CharacterDeescape((char*)NULL, '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
#endif	

	//String doesn't contain character to escape
	NextSubTest();
	string1 = new BString("Old");
	string1->CharacterDeescape("/a/nh/g/bhhgy/fgtuhjkb/", '-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/a/nh/g/bhhgy/fgtuhjkb/") == 0);
	delete string1;	
}


CppUnit::Test *StringEscapeTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringEscapeTest>
		StringEscapeTestCaller;
		
	return(new StringEscapeTestCaller("BString::Escape Test", &StringEscapeTest::PerformTest));
}
