#include "StringConstructionTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringConstructionTest::StringConstructionTest(std::string name)
		: BTestCase(name)
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
	
	// BString()
	NextSubTest();
	string = new BString;
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	CPPUNIT_ASSERT(string->Length() == 0);
	delete string;
	
	// BString(const char*)
	NextSubTest();
	string = new BString(str);
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT((unsigned)string->Length() == strlen(str));
	delete string;
	
	// BString(NULL)
	NextSubTest();
	string = new BString(NULL);
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	CPPUNIT_ASSERT(string->Length() == 0);
	delete string;
	
	// BString(BString&)
	NextSubTest();
	BString anotherString("Something Else");
	string = new BString(anotherString);
	CPPUNIT_ASSERT(strcmp(string->String(), anotherString.String()) == 0);
	CPPUNIT_ASSERT(string->Length() == anotherString.Length());
	delete string;
	
	// BString(const char*, int32)
	NextSubTest();
	string = new BString(str, 5);
	CPPUNIT_ASSERT(strcmp(string->String(), str) != 0);
	CPPUNIT_ASSERT(strncmp(string->String(), str, 5) == 0);
	CPPUNIT_ASSERT(string->Length() == 5);
	delete string;

	// BString(BString&&)
#if __cplusplus >= 201103L
	NextSubTest();
	BString movableString(str);
	string = new BString(std::move(movableString));
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT(string->Length() == strlen(str));
	CPPUNIT_ASSERT(strcmp(movableString.String(), "") == 0);
	CPPUNIT_ASSERT(movableString.Length() == 0);
	delete string;
#endif

	NextSubTest();
	string = new BString(str, 255);
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT((unsigned)string->Length() == strlen(str));
	delete string;	
}


CppUnit::Test *StringConstructionTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringConstructionTest>
		StringConstructionTestCaller;
		
	return(new StringConstructionTestCaller("BString::Construction Test",
		&StringConstructionTest::PerformTest));
}
