#include "StringCharAccessTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringCharAccessTest::StringCharAccessTest(std::string name) :
		BTestCase(name)
{
}

 

StringCharAccessTest::~StringCharAccessTest()
{
}


void 
StringCharAccessTest::PerformTest(void)
{
	BString string("A simple string");
	
	//operator[]
	NextSubTest();
	CPPUNIT_ASSERT(string[0] == 'A');
	CPPUNIT_ASSERT(string[1] == ' ');

	//&operator[]
	NextSubTest();
	string[0] = 'a';
	CPPUNIT_ASSERT(strcmp(string.String(), "a simple string") == 0);
	
	//ByteAt(int32)
	NextSubTest();
	CPPUNIT_ASSERT(string.ByteAt(-10) == 0);
	CPPUNIT_ASSERT(string.ByteAt(200) == 0);
	CPPUNIT_ASSERT(string.ByteAt(1) == ' ');
	CPPUNIT_ASSERT(string.ByteAt(7) == 'e');
}


CppUnit::Test *StringCharAccessTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringCharAccessTest>
		StringCharAccessTestCaller;
		
	return(new StringCharAccessTestCaller("BString::CharAccess Test", &StringCharAccessTest::PerformTest));
}
