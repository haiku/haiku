#include "StringAppendTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringAppendTest::StringAppendTest(std::string name)
		: BTestCase(name)
{
}

 
StringAppendTest::~StringAppendTest()
{
}


void 
StringAppendTest::PerformTest(void)
{
	BString *str1, *str2;
	
	// +=(BString&)
	NextSubTest();
	str1 = new BString("BASE");
	str2 = new BString("APPENDED");
	*str1 += *str2;
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAPPENDED") == 0);
	delete str1;
	delete str2;
	
	// +=(const char *)
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
	
	// char pointer is NULL
	NextSubTest();
	char *tmp = NULL;
	str1 = new BString("Base");
	*str1 += tmp;
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
	
	// +=(char)
	NextSubTest();
	str1 = new BString("Base");
	*str1 += 'C';
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseC") == 0);
	delete str1;
	
	// Append(BString&)
	NextSubTest();
	str1 = new BString("BASE");
	str2 = new BString("APPENDED");
	str1->Append(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAPPENDED") == 0);
	delete str1;
	delete str2;
	
	// Append(const char*)
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
	
	// char ptr is NULL
	NextSubTest();
	str1 = new BString("Base");
	str1->Append(tmp);
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
	
	// Append(BString&, int32)
	NextSubTest();
	str1 = new BString("BASE");
	str2 = new BString("APPENDED");
	str1->Append(*str2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAP") == 0);
	delete str1;
	delete str2;
	
	// Append(const char*, int32)
	NextSubTest();
	str1 = new BString("Base");
	str1->Append("APPENDED", 40);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	CPPUNIT_ASSERT(str1->Length() == (int32)strlen("BaseAPPENDED"));
	delete str1;
	
	// char ptr is NULL
	NextSubTest();
	str1 = new BString("BLABLA");
	str1->Append(tmp, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BLABLA") == 0);
	delete str1;
	
	// Append(char, int32)
	NextSubTest();
	str1 = new BString("Base");
	str1->Append('C', 5);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseCCCCC") == 0);
	delete str1;

	// TODO: The following test cases only work with hoard2, which will not
	// allow allocations via malloc() larger than the largest size-class
	// (see threadHeap::malloc(size_t). Other malloc implementations like
	// rpmalloc will allow arbitrarily large allocations via create_area().
	//
	// This test should be made more robust by breaking the dependency on
	// the allocator to simulate failures in another way. This may require
	// a tricky build configuration to avoid breaking the ABI of BString.
#ifndef TEST_R5
	const int32 OUT_OF_MEM_VAL = 2 * 1000 * 1000 * 1000;
	// Append(char, int32) with excessive length:
	NextSubTest();
	str1 = new BString("Base");
	str1->Append('C', OUT_OF_MEM_VAL);
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
#endif

#ifndef TEST_R5
	// Append(char*, int32) with excessive length:
	NextSubTest();
	str1 = new BString("Base");
	str1->Append("some more text", OUT_OF_MEM_VAL);
	CPPUNIT_ASSERT(strcmp(str1->String(), "Basesome more text") == 0);
	delete str1;
#endif
}


CppUnit::Test *StringAppendTest::suite(void)
{	
	typedef CppUnit::TestCaller<StringAppendTest>
		StringAppendTestCaller;
		
	return(new StringAppendTestCaller("BString::Append Test",
		&StringAppendTest::PerformTest));
}
