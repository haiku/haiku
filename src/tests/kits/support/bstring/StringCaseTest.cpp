#include "StringCaseTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringCaseTest::StringCaseTest(std::string name) :
		BTestCase(name)
{
}

 

StringCaseTest::~StringCaseTest()
{
}


void 
StringCaseTest::PerformTest(void)
{
	BString *string;
	
	//Capitalize
	NextSubTest();
	string = new BString("this is a sentence");
	string->Capitalize();
	CPPUNIT_ASSERT(strcmp(string->String(), "This is a sentence") == 0);
	delete string;
	
	NextSubTest();
	string = new BString("134this is a sentence");
	string->Capitalize();
	CPPUNIT_ASSERT(strcmp(string->String(), "134this is a sentence") == 0);
	delete string;

	NextSubTest();
	string = new BString;
	string->Capitalize();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;
	
	//ToLower
	NextSubTest();
	string = new BString("1a2B3c4d5e6f7G");
	string->ToLower();
	CPPUNIT_ASSERT(strcmp(string->String(), "1a2b3c4d5e6f7g") == 0);
	delete string;
	
	NextSubTest();
	string = new BString;
	string->ToLower();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;
	
	//ToUpper
	NextSubTest();
	string = new BString("1a2b3c4d5E6f7g");
	string->ToUpper();
	CPPUNIT_ASSERT(strcmp(string->String(), "1A2B3C4D5E6F7G") == 0);
	delete string;
	
	NextSubTest();
	string = new BString;
	string->ToUpper();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;
	
	//CapitalizeEachWord
	NextSubTest();
	string = new BString("each wOrd 3will_be >capiTalized");
	string->CapitalizeEachWord();
	CPPUNIT_ASSERT(strcmp(string->String(), "Each Word 3Will_Be >Capitalized") == 0);
	delete string;
	
	NextSubTest();
	string = new BString;
	string->CapitalizeEachWord();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;		
}


CppUnit::Test *StringCaseTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringCaseTest>
		StringCaseTestCaller;
		
	return(new StringCaseTestCaller("BString::Case Test", &StringCaseTest::PerformTest));
}
