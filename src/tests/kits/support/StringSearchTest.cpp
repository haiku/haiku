/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringSearchTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringSearchTest);
	CPPUNIT_TEST(FindFirst_BString_Found);
	CPPUNIT_TEST(FindFirst_BString_NotFound);
	CPPUNIT_TEST(FindFirst_CString_Found);
	CPPUNIT_TEST(FindFirst_CString_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(FindFirst_Null_ReturnsBadValue);
#endif
	CPPUNIT_TEST(FindFirst_BString_ValidOffset_Found);
	CPPUNIT_TEST(FindFirst_BString_OutOfBoundsOffset_NotFound);
	CPPUNIT_TEST(FindFirst_BString_NegativeOffset_NotFound);
	CPPUNIT_TEST(FindFirst_CString_ValidOffset_Found);
	CPPUNIT_TEST(FindFirst_CString_OutOfBoundsOffset_NotFound);
	CPPUNIT_TEST(FindFirst_CString_NegativeOffset_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(FindFirst_Null_ValidOffset_ReturnsBadValue);
#endif
	CPPUNIT_TEST(FindFirst_Char_Found);
	CPPUNIT_TEST(FindFirst_Char_NotFound);
	CPPUNIT_TEST(FindFirst_CString_ValidOffset_Found_1);
	CPPUNIT_TEST(FindFirst_Char_ValidOffset_Found);
	CPPUNIT_TEST(FindFirst_Char_ValidOffset_NotFound);
	CPPUNIT_TEST(FindFirst_Char_LastOffset_Found);
	CPPUNIT_TEST(FindFirst_Char_LastOffset_NotFound);
	CPPUNIT_TEST(FindFirst_CString_ValidOffset_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(StartsWith_BString_ReturnsTrue);
	CPPUNIT_TEST(StartsWith_CString_ReturnsTrue);
	CPPUNIT_TEST(StartsWith_CString_ValidOffset_ReturnsTrue);
#endif
	CPPUNIT_TEST(FindLast_BString_Found);
	CPPUNIT_TEST(FindLast_BString_NotFound);
	CPPUNIT_TEST(FindLast_CString_Found);
	CPPUNIT_TEST(FindLast_CString_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(FindLast_Null_ReturnsBadValue);
#endif
	CPPUNIT_TEST(FindLast_BString_ValidOffset_Found);
	CPPUNIT_TEST(FindLast_BString_NegativeOffset_NotFound);
	CPPUNIT_TEST(FindLast_CString_ValidOffset_Found);
	CPPUNIT_TEST(FindLast_CString_NegativeOffset_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(FindLast_Null_ValidOffset_ReturnsBadValue);
#endif
	CPPUNIT_TEST(FindLast_Char_Found);
	CPPUNIT_TEST(FindLast_Char_NotFound);
	CPPUNIT_TEST(FindLast_CString_ValidOffset_Found_1);
	CPPUNIT_TEST(FindLast_Char_ValidOffset_NotFound);
	CPPUNIT_TEST(FindLast_Char_ValidOffset_Found);
	CPPUNIT_TEST(FindLast_Char_ValidOffset_Found_1);
	CPPUNIT_TEST(FindLast_CString_ValidOffset_NotFound);
	CPPUNIT_TEST(IFindFirst_BString_Found);
	CPPUNIT_TEST(IFindFirst_BString_Found_1);
	CPPUNIT_TEST(IFindFirst_BString_NotFound);
	CPPUNIT_TEST(IFindFirst_BString_Found_2);
	CPPUNIT_TEST(IFindFirst_CString_Found);
	CPPUNIT_TEST(IFindFirst_CString_Found_1);
	CPPUNIT_TEST(IFindFirst_CString_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(IFindFirst_Null_ReturnsBadValue);
#endif
	CPPUNIT_TEST(IFindFirst_BString_ValidOffset_Found);
	CPPUNIT_TEST(IFindFirst_BString_ValidOffset_Found_1);
	CPPUNIT_TEST(IFindFirst_BString_OutOfBoundsOffset_NotFound);
	CPPUNIT_TEST(IFindFirst_BString_NegativeOffset_NotFound);
	CPPUNIT_TEST(IFindFirst_CString_ValidOffset_Found);
	CPPUNIT_TEST(IFindFirst_CString_ValidOffset_Found_1);
	CPPUNIT_TEST(IFindFirst_CString_OutOfBoundsOffset_NotFound);
	CPPUNIT_TEST(IFindFirst_CString_NegativeOffset_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(IStartsWith_BString_ReturnsTrue);
	CPPUNIT_TEST(IStartsWith_CString_ReturnsTrue);
	CPPUNIT_TEST(IStartsWith_CString_ValidOffset_ReturnsTrue);
	CPPUNIT_TEST(IFindLast_BString_Found);
	CPPUNIT_TEST(IFindLast_BString_Found_1);
	CPPUNIT_TEST(EndsWith_BString_ReturnsTrue);
	CPPUNIT_TEST(EndsWith_BString_ReturnsFalse);
	CPPUNIT_TEST(EndsWith_CString_ReturnsTrue);
	CPPUNIT_TEST(EndsWith_CString_ReturnsFalse);
	CPPUNIT_TEST(EndsWith_CString_ValidOffset_ReturnsTrue);
	CPPUNIT_TEST(EndsWith_CString_ValidOffset_ReturnsFalse);
	CPPUNIT_TEST(IEndsWith_BString_ReturnsTrue);
	CPPUNIT_TEST(IEndsWith_BString_ReturnsTrue_1);
	CPPUNIT_TEST(IEndsWith_CString_ReturnsTrue);
	CPPUNIT_TEST(IEndsWith_CString_ReturnsTrue_1);
	CPPUNIT_TEST(IEndsWith_CString_ValidOffset_ReturnsTrue);
	CPPUNIT_TEST(IEndsWith_CString_ValidOffset_ReturnsTrue_1);
#endif
	CPPUNIT_TEST(IFindLast_BString_NotFound);
	CPPUNIT_TEST(IFindLast_CString_Found);
	CPPUNIT_TEST(IFindLast_CString_NotFound);
#ifndef TEST_R5
	CPPUNIT_TEST(IFindLast_CString_Found_1);
	CPPUNIT_TEST(IFindLast_Null_ReturnsBadValue);
	CPPUNIT_TEST(IFindLast_CString_ValidOffset_Found_1);
#endif
	CPPUNIT_TEST(IFindLast_BString_ValidOffset_Found);
	CPPUNIT_TEST(IFindLast_BString_ValidOffset_Found_1);
	CPPUNIT_TEST(IFindLast_BString_NegativeOffset_NotFound);
	CPPUNIT_TEST(IFindLast_CString_ValidOffset_Found);
	CPPUNIT_TEST(IFindLast_CString_NegativeOffset_NotFound);
	CPPUNIT_TEST(IFindLast_CString_ValidOffset_Found_2);
	CPPUNIT_TEST_SUITE_END();

public:
	void FindFirst_BString_Found()
	{
		BString string1("last but not least");
		BString string2("st");
		CPPUNIT_ASSERT_EQUAL(2, string1.FindFirst(string2));
	}

	void FindFirst_BString_NotFound()
	{
		BString string1;
		BString string2("some text");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst(string2));
	}

	void FindFirst_CString_Found()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT_EQUAL(2, string1.FindFirst("st"));
	}

	void FindFirst_CString_NotFound()
	{
		BString string1;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst("some text"));
	}

#ifndef TEST_R5
	void FindFirst_Null_ReturnsBadValue()
	{
		BString string1("string");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, string1.FindFirst((char*)NULL));
	}
#endif

	void FindFirst_BString_ValidOffset_Found()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(8, string1.FindFirst(string2, 5));
	}

	void FindFirst_BString_OutOfBoundsOffset_NotFound()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst(string2, 200));
	}

	void FindFirst_BString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst(string2, -10));
	}

	void FindFirst_CString_ValidOffset_Found()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(4, string1.FindFirst("abc", 2));
	}

	void FindFirst_CString_OutOfBoundsOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst("abc", 200));
	}

	void FindFirst_CString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst("abc", -10));
	}

#ifndef TEST_R5
	void FindFirst_Null_ValidOffset_ReturnsBadValue()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, string1.FindFirst((char*)NULL, 3));
	}
#endif

	void FindFirst_Char_Found()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(2, string1.FindFirst('c'));
	}

	void FindFirst_Char_NotFound()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst('e'));
	}

	void FindFirst_CString_ValidOffset_Found_1()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(5, string1.FindFirst("b", 3));
	}

	void FindFirst_Char_ValidOffset_Found()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(5, string1.FindFirst('a', 3));
	}

	void FindFirst_Char_ValidOffset_NotFound()
	{
		BString string1;
		string1.SetTo('e', 200);
		string1.SetTo('a', 190);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst('e', 185));
	}

	void FindFirst_Char_LastOffset_Found()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(8, string1.FindFirst('d', 8));
	}

	void FindFirst_Char_LastOffset_NotFound()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst('e', 8));
	}

	void FindFirst_CString_ValidOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindFirst("a", 9));
	}

#ifndef TEST_R5
	void StartsWith_BString_ReturnsTrue()
	{
		BString string1("last but not least");
		BString string2("last");
		CPPUNIT_ASSERT(string1.StartsWith(string2));
	}

	void StartsWith_CString_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.StartsWith("last"));
	}

	void StartsWith_CString_ValidOffset_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.StartsWith("last", 4));
	}
#endif

	void FindLast_BString_Found()
	{
		BString string1("last but not least");
		BString string2("st");
		CPPUNIT_ASSERT_EQUAL(16, string1.FindLast(string2));
	}

	void FindLast_BString_NotFound()
	{
		BString string1;
		BString string2("some text");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast(string2));
		// FindLast(char*)
	}

	void FindLast_CString_Found()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT_EQUAL(16, string1.FindLast("st"));
	}

	void FindLast_CString_NotFound()
	{
		BString string1;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast("some text"));
	}

#ifndef TEST_R5
	void FindLast_Null_ReturnsBadValue()
	{
		BString string1("string");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, string1.FindLast((char*)NULL));
	}
#endif

	void FindLast_BString_ValidOffset_Found()
	{
		BString string1("abcabcabc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(3, string1.FindLast(string2, 7));
	}

	void FindLast_BString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast(string2, -10));
		// FindLast(const char*, int32)
	}

	void FindLast_CString_ValidOffset_Found()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(4, string1.FindLast("abc", 9));
	}

	void FindLast_CString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast("abc", -10));
	}

#ifndef TEST_R5
	void FindLast_Null_ValidOffset_ReturnsBadValue()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, string1.FindLast((char*)NULL, 3));
	}
#endif

	void FindLast_Char_Found()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(7, string1.FindLast('c'));
	}

	void FindLast_Char_NotFound()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast('e'));
	}

	void FindLast_CString_ValidOffset_Found_1()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(1, string1.FindLast("b", 5));
	}

	void FindLast_Char_ValidOffset_NotFound()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast('e', 3));
	}

	void FindLast_Char_ValidOffset_Found()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(6, string1.FindLast('b', 6));
	}

	void FindLast_Char_ValidOffset_Found_1()
	{
		BString string1("abcd abcd");
		CPPUNIT_ASSERT_EQUAL(1, string1.FindLast('b', 5));
	}

	void FindLast_CString_ValidOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.FindLast("a", 0));
	}

	void IFindFirst_BString_Found()
	{
		BString string1("last but not least");
		BString string2("st");
		CPPUNIT_ASSERT_EQUAL(2, string1.IFindFirst(string2));
	}

	void IFindFirst_BString_Found_1()
	{
		BString string1("last but not least");
		BString string2("ST");
		CPPUNIT_ASSERT_EQUAL(2, string1.IFindFirst(string2));
	}

	void IFindFirst_BString_NotFound()
	{
		BString string1;
		BString string2("some text");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindFirst(string2));
	}

	void IFindFirst_BString_Found_2()
	{
		BString string1("string");
		BString string2;
		CPPUNIT_ASSERT_EQUAL(0, string1.IFindFirst(string2));
	}

	void IFindFirst_CString_Found()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT_EQUAL(2, string1.IFindFirst("st"));
	}

	void IFindFirst_CString_Found_1()
	{
		BString string1("LAST BUT NOT least");
		CPPUNIT_ASSERT_EQUAL(2, string1.IFindFirst("st"));
	}

	void IFindFirst_CString_NotFound()
	{
		BString string1;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindFirst("some text"));
	}

#ifndef TEST_R5
	void IFindFirst_Null_ReturnsBadValue()
	{
		BString string1("string");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, string1.IFindFirst((char*)NULL));
	}
#endif

	void IFindFirst_BString_ValidOffset_Found()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(8, string1.IFindFirst(string2, 5));
	}

	void IFindFirst_BString_ValidOffset_Found_1()
	{
		BString string1("abc abc abc");
		BString string2("AbC");
		CPPUNIT_ASSERT_EQUAL(8, string1.IFindFirst(string2, 5));
	}

	void IFindFirst_BString_OutOfBoundsOffset_NotFound()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindFirst(string2, 200));
	}

	void IFindFirst_BString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindFirst(string2, -10));
	}

	void IFindFirst_CString_ValidOffset_Found()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(4, string1.IFindFirst("abc", 2));
	}

	void IFindFirst_CString_ValidOffset_Found_1()
	{
		BString string1("AbC ABC abC");
		CPPUNIT_ASSERT_EQUAL(4, string1.IFindFirst("abc", 2));
	}

	void IFindFirst_CString_OutOfBoundsOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindFirst("abc", 200));
	}

	void IFindFirst_CString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindFirst("abc", -10));
	}
#ifndef TEST_R5
	void IStartsWith_BString_ReturnsTrue()
	{
		BString string1("last but not least");
		BString string2("lAsT");
		CPPUNIT_ASSERT(string1.IStartsWith(string2));
	}

	void IStartsWith_CString_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.IStartsWith("lAsT"));
	}

	void IStartsWith_CString_ValidOffset_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.IStartsWith("lAsT", 4));
	}

	void IFindLast_BString_Found()
	{
		BString string1("last but not least");
		BString string2("st");
		CPPUNIT_ASSERT_EQUAL(16, string1.IFindLast(string2));
	}

	void IFindLast_BString_Found_1()
	{
		BString string1("laSt but NOT leaSt");
		BString string2("sT");
		CPPUNIT_ASSERT_EQUAL(16, string1.IFindLast(string2));
	}

	void EndsWith_BString_ReturnsTrue()
	{
		BString string1("last but not least");
		BString string2("st");
		CPPUNIT_ASSERT(string1.EndsWith(string2));
	}

	void EndsWith_BString_ReturnsFalse()
	{
		BString string1("laSt but NOT leaSt");
		BString string2("sT");
		CPPUNIT_ASSERT_EQUAL(0, string1.EndsWith(string2));
	}

	void EndsWith_CString_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.EndsWith("least"));
	}

	void EndsWith_CString_ReturnsFalse()
	{
		BString string1("laSt but NOT leaSt");
		CPPUNIT_ASSERT_EQUAL(0, string1.EndsWith("least"));
	}

	void EndsWith_CString_ValidOffset_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.EndsWith("st", 2));
	}

	void EndsWith_CString_ValidOffset_ReturnsFalse()
	{
		BString string1("laSt but NOT leaSt");
		CPPUNIT_ASSERT_EQUAL(0, string1.EndsWith("sT", 2));
	}

	void IEndsWith_BString_ReturnsTrue()
	{
		BString string1("last but not least");
		BString string2("st");
		CPPUNIT_ASSERT(string1.IEndsWith(string2));
	}

	void IEndsWith_BString_ReturnsTrue_1()
	{
		BString string1("laSt but NOT leaSt");
		BString string2("sT");
		CPPUNIT_ASSERT(string1.IEndsWith(string2));
	}

	void IEndsWith_CString_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.IEndsWith("st"));
	}

	void IEndsWith_CString_ReturnsTrue_1()
	{
		BString string1("laSt but NOT leaSt");
		CPPUNIT_ASSERT(string1.IEndsWith("sT"));
	}

	void IEndsWith_CString_ValidOffset_ReturnsTrue()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT(string1.IEndsWith("st", 2));
	}

	void IEndsWith_CString_ValidOffset_ReturnsTrue_1()
	{
		BString string1("laSt but NOT leaSt");
		CPPUNIT_ASSERT(string1.IEndsWith("sT", 2));
	}
#endif

	void IFindLast_BString_NotFound()
	{
		BString string1;
		BString string2("some text");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindLast(string2));
	}

	void IFindLast_CString_Found()
	{
		BString string1("last but not least");
		CPPUNIT_ASSERT_EQUAL(16, string1.IFindLast("st"));
	}

	void IFindLast_CString_NotFound()
	{
		BString string1;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindLast("some text"));
	}

#ifndef TEST_R5
	void IFindLast_CString_Found_1()
	{
		BString string1("laSt but NOT leaSt");
		CPPUNIT_ASSERT_EQUAL(16, string1.IFindLast("ST"));
	}

	void IFindLast_Null_ReturnsBadValue()
	{
		BString string1("string");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, string1.IFindLast((char*)NULL));
	}

	void IFindLast_CString_ValidOffset_Found_1()
	{
		BString string1("ABc abC aBC");
		CPPUNIT_ASSERT_EQUAL(4, string1.IFindLast("aBc", 9));
	}
#endif

	void IFindLast_BString_ValidOffset_Found()
	{
		BString string1("abcabcabc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(3, string1.IFindLast(string2, 7));
	}

	void IFindLast_BString_ValidOffset_Found_1()
	{
		BString string1("abcabcabc");
		BString string2("AbC");
		CPPUNIT_ASSERT_EQUAL(3, string1.IFindLast(string2, 7));
	}

	void IFindLast_BString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		BString string2("abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindLast(string2, -10));
	}

	void IFindLast_CString_ValidOffset_Found()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(4, string1.IFindLast("abc", 9));
	}

	void IFindLast_CString_NegativeOffset_NotFound()
	{
		BString string1("abc abc abc");
		CPPUNIT_ASSERT_EQUAL(B_ERROR, string1.IFindLast("abc", -10));
	}

	void IFindLast_CString_ValidOffset_Found_2()
	{
		BString string1("abc def ghi");
		CPPUNIT_ASSERT_EQUAL(0, string1.IFindLast("abc", 4));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringSearchTest, getTestSuiteName());
