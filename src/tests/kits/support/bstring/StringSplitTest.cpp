#include "StringSplitTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <StringList.h>


StringSplitTest::StringSplitTest(std::string name) 
		: BTestCase(name)
{
}


StringSplitTest::~StringSplitTest()
{
}


void
StringSplitTest::PerformTest(void)
{
	BString *str1;

	NextSubTest();
	BStringList stringList1;
	str1 = new BString("test::string");
	str1->Split(":", true, stringList1);
	CPPUNIT_ASSERT_EQUAL(2, stringList1.CountStrings());
	delete str1;

	NextSubTest();
	BStringList stringList2;
	str1 = new BString("test::string");
	str1->Split("::", true, stringList2);
	CPPUNIT_ASSERT_EQUAL(2, stringList2.CountStrings());
	delete str1;

	NextSubTest();
	BStringList stringList3;
	str1 = new BString("test::string");
	str1->Split("::", false, stringList3);
	CPPUNIT_ASSERT_EQUAL(2, stringList3.CountStrings());
	delete str1;

	NextSubTest();
	BStringList stringList4;
	str1 = new BString("test::string");
	str1->Split(":", false, stringList4);
	CPPUNIT_ASSERT_EQUAL(3, stringList4.CountStrings());
	delete str1;

}


CppUnit::Test *StringSplitTest::suite(void)
{
	typedef CppUnit::TestCaller<StringSplitTest>
		StringSplitTestCaller;

	return(new StringSplitTestCaller("BString::Split Test",
		&StringSplitTest::PerformTest));
}
