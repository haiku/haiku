#include "StringCompareTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringCompareTest::StringCompareTest(std::string name) :
		BTestCase(name)
{
}

 

StringCompareTest::~StringCompareTest()
{
}


void 
StringCompareTest::PerformTest(void)
{
	BString *string1, *string2;
	
	//operator<(const BString &) const;
	NextSubTest();
	string1 = new BString("11111_a");
	string2 = new BString("22222_b");
	CPPUNIT_ASSERT(*string1 < *string2);
	delete string1;
	delete string2;
	
	//operator<=(const BString &) const;
	NextSubTest();
	string1 = new BString("11111_a");
	string2 = new BString("22222_b");
	CPPUNIT_ASSERT(*string1 <= *string2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("11111");
	string2 = new BString("11111");
	CPPUNIT_ASSERT(*string1 <= *string2);
	delete string1;
	delete string2;
	
	//operator==(const BString &) const;
	NextSubTest();
	string1 = new BString("string");
	string2 = new BString("string");
	CPPUNIT_ASSERT(*string1 == *string2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("text");
	string2 = new BString("string");
	CPPUNIT_ASSERT((*string1 == *string2) == false);
	delete string1;
	delete string2;
	
	//operator>=(const BString &) const;
	NextSubTest();
	string1 = new BString("BBBBB");
	string2 = new BString("AAAAA");
	CPPUNIT_ASSERT(*string1 >= *string2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("11111");
	string2 = new BString("11111");
	CPPUNIT_ASSERT(*string1 >= *string2);
	delete string1;
	delete string2;
	
	//operator>(const BString &) const;
	NextSubTest();
	string1 = new BString("BBBBB");
	string2 = new BString("AAAAA");
	CPPUNIT_ASSERT(*string1 > *string2);
	delete string1;
	delete string2;
	
	//operator!=(const BString &) const;
	NextSubTest();
	string1 = new BString("string");
	string2 = new BString("string");
	CPPUNIT_ASSERT((*string1 != *string2) == false);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BString("text");
	string2 = new BString("string");
	CPPUNIT_ASSERT(*string1 != *string2);
	delete string1;
	delete string2;
	
	//operator<(const char *) const;
	NextSubTest();
	string1 = new BString("AAAAA");
	CPPUNIT_ASSERT(*string1 < "BBBBB");
	delete string1;
	
	//operator<=(const char *) const;
	NextSubTest();
	string1 = new BString("AAAAA");
	CPPUNIT_ASSERT(*string1 <= "BBBBB");
	CPPUNIT_ASSERT(*string1 <= "AAAAA");
	delete string1;
	
	//operator==(const char *) const;
	NextSubTest();
	string1 = new BString("AAAAA");
	CPPUNIT_ASSERT(*string1 == "AAAAA");
	delete string1;
	
	NextSubTest();
	string1 = new BString("AAAAA");
	CPPUNIT_ASSERT((*string1 == "BBBB") == false);
	delete string1;
	
	//operator>=(const char *) const;
	NextSubTest();
	string1 = new BString("BBBBB");
	CPPUNIT_ASSERT(*string1 >= "AAAAA");
	CPPUNIT_ASSERT(*string1 >= "BBBBB");
	delete string1;
	
	//operator>(const char *) const;
	NextSubTest();
	string1 = new BString("BBBBB");
	CPPUNIT_ASSERT(*string1 > "AAAAA");
	delete string1;
	
	//operator!=(const char *) const;
	NextSubTest();
	string1 = new BString("AAAAA");
	CPPUNIT_ASSERT((*string1 != "AAAAA") == false);
	delete string1;
	
	NextSubTest();
	string1 = new BString("AAAAA");
	CPPUNIT_ASSERT(*string1 != "BBBB");
	delete string1;
}


CppUnit::Test *StringCompareTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringCompareTest>
		StringCompareTestCaller;
		
	return(new StringCompareTestCaller("BString::Compare Test", &StringCompareTest::PerformTest));
}
