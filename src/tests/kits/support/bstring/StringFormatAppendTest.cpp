#include "StringFormatAppendTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringFormatAppendTest::StringFormatAppendTest(std::string name) :
		BTestCase(name)
{
}

 

StringFormatAppendTest::~StringFormatAppendTest()
{
}


void 
StringFormatAppendTest::PerformTest(void)
{
	BString *string, *string2;
	
	//operator<<(const char *);
	NextSubTest();
	string = new BString("some");
	*string << " ";
	*string << "text";
	CPPUNIT_ASSERT(strcmp(string->String(), "some text") == 0);
	delete string;
	
	//operator<<(const BString &);
	NextSubTest();
	string = new BString("some ");
	string2 = new BString("text");
	*string << *string2;
	CPPUNIT_ASSERT(strcmp(string->String(), "some text") == 0);
	delete string;
	delete string2;
	
	//operator<<(char);
	NextSubTest();
	string = new BString("str");
	*string << 'i' << 'n' << 'g';
	CPPUNIT_ASSERT(strcmp(string->String(), "string") == 0);
	delete string;
		
	//operator<<(int);
	NextSubTest();
	string = new BString("level ");
	*string << (int)42;
	CPPUNIT_ASSERT(strcmp(string->String(), "level 42") == 0);
	delete string;
	
	NextSubTest();
	string = new BString("error ");
	*string << (int)-1;
	CPPUNIT_ASSERT(strcmp(string->String(), "error -1") == 0);
	delete string;
	
	//operator<<(unsigned int);
	NextSubTest();
	string = new BString("number ");
	*string << (unsigned int)296;
	CPPUNIT_ASSERT(strcmp(string->String(), "number 296") == 0);
	delete string;
	
	//operator<<(uint32);
	NextSubTest();
	string = new BString;
	*string << (uint32)102456;
	CPPUNIT_ASSERT(strcmp(string->String(), "102456") == 0);
	delete string;

	//operator<<(int32);
	NextSubTest();
	string = new BString;
	*string << (int32)112456;
	CPPUNIT_ASSERT(strcmp(string->String(), "112456") == 0);
	delete string;

	NextSubTest();
	string = new BString;
	*string << (int32)-112475;
	CPPUNIT_ASSERT(strcmp(string->String(), "-112475") == 0);
	delete string;

	//operator<<(uint64);
	NextSubTest();
	string = new BString;
	*string << (uint64)1145267987;
	CPPUNIT_ASSERT(strcmp(string->String(), "1145267987") == 0);
	delete string;

	//operator<<(int64);
	NextSubTest();
	string = new BString;
	*string << (int64)112456;
	CPPUNIT_ASSERT(strcmp(string->String(), "112456") == 0);
	delete string;

	NextSubTest();
	string = new BString;
	*string << (int64)-112475;
	CPPUNIT_ASSERT(strcmp(string->String(), "-112475") == 0);
	delete string;

	//operator<<(float);
	NextSubTest();
	string = new BString;
	*string << (float)34.542;
	CPPUNIT_ASSERT(strcmp(string->String(), "34.54") == 0);
	delete string;
	
	//Misc test
	NextSubTest();
	BString s;
	s << "This" << ' ' << "is" << ' ' << 'a' << ' ' << "test" << ' ' << "sentence";		
	CPPUNIT_ASSERT(strcmp(s.String(), "This is a test sentence") == 0);
}


CppUnit::Test *StringFormatAppendTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringFormatAppendTest>
		StringFormatAppendTestCaller;
		
	return(new StringFormatAppendTestCaller("BString::FormatAppend Test", &StringFormatAppendTest::PerformTest));
}
