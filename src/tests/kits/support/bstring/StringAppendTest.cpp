#include "StringAppendTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringAppendTest::StringAppendTest(std::string name) :
		BTestCase(name)
{
}

 

StringAppendTest::~StringAppendTest()
{
}


void 
StringAppendTest::PerformTest(void)
{
	BString *str1, *str2;
	
	NextSubTest();
	str1 = new BString("BASE");
	str2 = new BString("APPENDED");
	*str1 += *str2;
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAPPENDED") == 0);
	delete str1;
	delete str2;
	
	NextSubTest();
	str1 = new BString("Base");
	*str1 += "APPENDED";
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString;
	*str1 += "APPENDEDTONOTHING";
	CPPUNIT_ASSERT(strcmp(str1->String(), "APPENDEDTONOTHING") == 0);
	delete str1;
	
	NextSubTest();
	char *tmp = NULL;
	str1 = new BString("Base");
	*str1 += tmp;
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString("Base");
	*str1 += 'C';
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseC") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("BASE");
	str2 = new BString("APPENDED");
	str1->Append(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAPPENDED") == 0);
	delete str1;
	delete str2;
	
	NextSubTest();
	str1 = new BString("Base");
	str1->Append("APPENDED");
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString;
	str1->Append("APPENDEDTONOTHING");
	CPPUNIT_ASSERT(strcmp(str1->String(), "APPENDEDTONOTHING") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString("Base");
	str1->Append(tmp);
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString("BASE");
	str2 = new BString("APPENDED");
	str1->Append(*str2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAP") == 0);
	delete str1;
	delete str2;
	
	NextSubTest();
	str1 = new BString("Base");
	str1->Append("APPENDED", 40);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	CPPUNIT_ASSERT(str1->Length() == strlen("BaseAPPENDED"));
	delete str1;
	
	NextSubTest();
	str1 = new BString("BLABLA");
	str1->Append(tmp, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BLABLA") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BString("Base");
	str1->Append('C', 5);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseCCCCC") == 0);
	delete str1;
}


CppUnit::Test *StringAppendTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringAppendTest>
		StringAppendTestCaller;
		
	return(new StringAppendTestCaller("BString::Append Test", &StringAppendTest::PerformTest));
}
