#include "StringAssignTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>


StringAssignTest::StringAssignTest(std::string name)
		: BTestCase(name)
{
}


StringAssignTest::~StringAssignTest()
{
}


void
StringAssignTest::PerformTest(void)
{
	// =(BString&)
	NextSubTest();
	BString string;
	BString string2("Something");
	string = string2;
	CPPUNIT_ASSERT(strcmp(string.String(), string2.String()) == 0);
	CPPUNIT_ASSERT(strcmp(string.String(), "Something") == 0);

	// =(const char*)
	NextSubTest();
	BString *str = new BString();
	*str = "Something Else";
	CPPUNIT_ASSERT(strcmp(str->String(), "Something Else") == 0);
	delete str;

	// char ptr is NULL
	NextSubTest();
	char *s = NULL;
	str = new BString;
	*str = s;
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;

	// SetTo(const char *) (NULL)
	NextSubTest();
	str = new BString;
	str->SetTo(s);
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;

	NextSubTest();
	str = new BString;
	str->SetTo("BLA");
	CPPUNIT_ASSERT(strcmp(str->String(), "BLA") == 0);
	delete str;

	// SetTo(BString&)
	NextSubTest();
	str = new BString;
	str->SetTo(string);
	CPPUNIT_ASSERT(strcmp(str->String(), string.String()) == 0);
	delete str;

	// SetTo(char, int32)
	NextSubTest();
	str = new BString;
	str->SetTo('C', 10);
	CPPUNIT_ASSERT(strcmp(str->String(), "CCCCCCCCCC") == 0);
	delete str;

	NextSubTest();
	str = new BString("ASDSGAFA");
	str->SetTo('C', 0);
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;

	// SetTo(const char*, int32)
	NextSubTest();
	str = new BString;
	str->SetTo("ABC", 10);
	CPPUNIT_ASSERT(strcmp(str->String(), "ABC") == 0);
	delete str;

	// Adopt(BString&)
	NextSubTest();
	const char *oldString2 = string2.String();
	str = new BString;
	str->Adopt(string2);
	CPPUNIT_ASSERT(strcmp(str->String(), oldString2) == 0);
	CPPUNIT_ASSERT(strcmp(string2.String(), "") == 0);
	delete str;

	NextSubTest();
	BString newstring("SomethingElseAgain");
	str = new BString;
	str->Adopt(newstring, 2);
	CPPUNIT_ASSERT(strncmp(str->String(), "SomethingElseAgain", 2) == 0);
	CPPUNIT_ASSERT(str->Length() == 2);
	CPPUNIT_ASSERT(strcmp(newstring.String(), "") == 0);
	delete str;

#ifndef TEST_R5
	// TODO: The following test cases only work with hoard2, which will not
	// allow allocations via malloc() larger than the largest size-class
	// (see threadHeap::malloc(size_t). Other malloc implementations like
	// rpmalloc will allow arbitrarily large allocations via create_area().
	//
	// This test should be made more robust by breaking the dependency on
	// the allocator to simulate failures in another way. This may require
	// a tricky build configuration to avoid breaking the ABI of BString.
	const int32 OUT_OF_MEM_VAL = 2 * 1000 * 1000 * 1000;
	// SetTo(char, int32) with excessive length:
	NextSubTest();
	str = new BString("dummy");
	str->SetTo('C', OUT_OF_MEM_VAL);
	CPPUNIT_ASSERT(strcmp(str->String(), "dummy") == 0);
	delete str;

	// SetTo(char*, int32) with excessive length:
	NextSubTest();
	str = new BString("dummy");
	str->SetTo("some more text", OUT_OF_MEM_VAL);
	CPPUNIT_ASSERT(strcmp(str->String(), "some more text") == 0);
	delete str;
#endif
}


CppUnit::Test *StringAssignTest::suite(void)
{
	typedef CppUnit::TestCaller<StringAssignTest>
		StringAssignTestCaller;

	return(new StringAssignTestCaller("BString::Assign Test",
		&StringAssignTest::PerformTest));
}
