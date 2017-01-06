#include "StringSearchTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>
#include <stdio.h>

StringSearchTest::StringSearchTest(std::string name) :
		BTestCase(name)
{
}



StringSearchTest::~StringSearchTest()
{
}


void
StringSearchTest::PerformTest(void)
{
	BString *string1, *string2;
	int32 i;

	//FindFirst(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("st");
	i = string1->FindFirst(*string2);
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString;
	string2 = new BString("some text");
	i = string1->FindFirst(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindFirst(char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = string1->FindFirst("st");
	CPPUNIT_ASSERT(i == 2);
	delete string1;

	NextSubTest();
	string1 = new BString;
	i = string1->FindFirst("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
//	Commented, since crashes R5
	NextSubTest();
	string1 = new BString("string");
	i = string1->FindFirst((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindFirst(BString&, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->FindFirst(*string2, 5);
	CPPUNIT_ASSERT(i == 8);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->FindFirst(*string2, 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->FindFirst(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindFirst(const char*, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindFirst("abc", 2);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindFirst("abc", 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindFirst("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented since crashes R5
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindFirst((char*)NULL, 3);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindFirst(char)
	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindFirst('c');
	CPPUNIT_ASSERT(i == 2);
	delete string1;

	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindFirst('e');
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//FindFirst(char, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindFirst("b", 3);
	CPPUNIT_ASSERT(i == 5);
	delete string1;

	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindFirst('e', 3);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindFirst("a", 9);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//StartsWith(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("last");
	i = (int32)string1->StartsWith(*string2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
	delete string2;

	//StartsWith(const char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->StartsWith("last");
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	//StartsWith(const char*, int32)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->StartsWith("last", 4);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
#endif

	//FindLast(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("st");
	i = string1->FindLast(*string2);
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString;
	string2 = new BString("some text");
	i = string1->FindLast(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindLast(char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = string1->FindLast("st");
	CPPUNIT_ASSERT(i == 16);
	delete string1;

	NextSubTest();
	string1 = new BString;
	i = string1->FindLast("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented since crashes R5
	NextSubTest();
	string1 = new BString("string");
	i = string1->FindLast((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindLast(BString&, int32)
	NextSubTest();
	string1 = new BString("abcabcabc");
	string2 = new BString("abc");
	i = string1->FindLast(*string2, 7);
	CPPUNIT_ASSERT(i == 3);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->FindLast(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindLast(const char*, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindLast("abc", 9);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindLast("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented since crashes r5
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindLast((char*)NULL, 3);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindLast(char)
	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindLast('c');
	CPPUNIT_ASSERT(i == 7);
	delete string1;

	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindLast('e');
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//FindLast(char, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindLast("b", 5);
	CPPUNIT_ASSERT(i == 1);
	delete string1;

	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindLast('e', 3);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindLast('b', 6);
	CPPUNIT_ASSERT(i == 6);
	delete string1;

	NextSubTest();
	string1 = new BString("abcd abcd");
	i = string1->FindLast('b', 5);
	CPPUNIT_ASSERT(i == 1);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->FindLast("a", 0);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//IFindFirst(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("st");
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("ST");
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString;
	string2 = new BString("some text");
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("string");
	string2 = new BString;
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == 0);
	delete string1;
	delete string2;

	//IFindFirst(const char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = string1->IFindFirst("st");
	CPPUNIT_ASSERT(i == 2);
	delete string1;

	NextSubTest();
	string1 = new BString("LAST BUT NOT least");
	i = string1->IFindFirst("st");
	CPPUNIT_ASSERT(i == 2);
	delete string1;

	NextSubTest();
	string1 = new BString;
	i = string1->IFindFirst("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented, since crashes R5
	NextSubTest();
	string1 = new BString("string");
	i = string1->IFindFirst((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//IFindFirst(BString&, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->IFindFirst(*string2, 5);
	CPPUNIT_ASSERT(i == 8);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("AbC");
	i = string1->IFindFirst(*string2, 5);
	CPPUNIT_ASSERT(i == 8);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->IFindFirst(*string2, 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->IFindFirst(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//IFindFirst(const char*, int32)
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->IFindFirst("abc", 2);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BString("AbC ABC abC");
	i = string1->IFindFirst("abc", 2);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->IFindFirst("abc", 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->IFindFirst("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//IStartsWith(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("lAsT");
	i = (int32)string1->StartsWith(*string2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
	delete string2;

	//IStartsWith(const char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->StartsWith("lAsT");
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	//IStartsWith(const char*, int32)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->StartsWith("lAsT", 4);
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	//IFindLast(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("st");
	i = string1->IFindLast(*string2);
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	string2 = new BString("sT");
	i = string1->IFindLast(*string2);
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	delete string2;

	//EndsWith(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("st");
	i = (int32)string1->EndsWith(*string2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	string2 = new BString("sT");
	i = (int32)string1->EndsWith(*string2);
	CPPUNIT_ASSERT(i == 0);
	delete string1;
	delete string2;

	//EndsWith(const char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->EndsWith("least");
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	i = (int32)string1->EndsWith("least");
	CPPUNIT_ASSERT(i == 0);
	delete string1;

	//EndsWith(const char*, int32)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->EndsWith("sT", 2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	i = (int32)string1->EndsWith("sT", 2);
	CPPUNIT_ASSERT(i == 0);
	delete string1;

	//IEndsWith(BString&)
	NextSubTest();
	string1 = new BString("last but not least");
	string2 = new BString("st");
	i = (int32)string1->IEndsWith(*string2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	string2 = new BString("sT");
	i = (int32)string1->IEndsWith(*string2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
	delete string2;

	//IEndsWith(const char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->IEndsWith("st");
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	i = (int32)string1->IEndsWith("sT");
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	//IEndsWith(const char*, int32)
	NextSubTest();
	string1 = new BString("last but not least");
	i = (int32)string1->IEndsWith("st", 2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;

	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	i = (int32)string1->IEndsWith("sT", 2);
	CPPUNIT_ASSERT(i != 0);
	delete string1;
#endif

	NextSubTest();
	string1 = new BString;
	string2 = new BString("some text");
	i = string1->IFindLast(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//IFindLast(const char*)
	NextSubTest();
	string1 = new BString("last but not least");
	i = string1->IFindLast("st");
	CPPUNIT_ASSERT(i == 16);
	delete string1;

#ifndef TEST_R5
	NextSubTest();
	string1 = new BString("laSt but NOT leaSt");
	i = string1->IFindLast("ST");
	CPPUNIT_ASSERT(i == 16);
	delete string1;
#endif

	NextSubTest();
	string1 = new BString;
	i = string1->IFindLast("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented since crashes R5
	NextSubTest();
	string1 = new BString("string");
	i = string1->IFindLast((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindLast(BString&, int32)
	NextSubTest();
	string1 = new BString("abcabcabc");
	string2 = new BString("abc");
	i = string1->IFindLast(*string2, 7);
	CPPUNIT_ASSERT(i == 3);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abcabcabc");
	string2 = new BString("AbC");
	i = string1->IFindLast(*string2, 7);
	CPPUNIT_ASSERT(i == 3);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BString("abc abc abc");
	string2 = new BString("abc");
	i = string1->IFindLast(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//IFindLast(const char*, int32)
//#ifndef TEST_R5
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->IFindLast("abc", 9);
	CPPUNIT_ASSERT(i == 4);
	delete string1;
//#endif
#ifndef TEST_R5
	NextSubTest();
	string1 = new BString("ABc abC aBC");
	i = string1->IFindLast("aBc", 9);
	CPPUNIT_ASSERT(i == 4);
	delete string1;
#endif
	NextSubTest();
	string1 = new BString("abc abc abc");
	i = string1->IFindLast("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BString("abc def ghi");
	i = string1->IFindLast("abc",4);
	CPPUNIT_ASSERT(i == 0);
	delete string1;
}


CppUnit::Test *StringSearchTest::suite(void)
{
	typedef CppUnit::TestCaller<StringSearchTest>
		StringSearchTestCaller;

	return(new StringSearchTestCaller("BString::Search Test", &StringSearchTest::PerformTest));
}
