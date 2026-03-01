/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringReplaceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringReplaceTest);
	CPPUNIT_TEST(ReplaceFirst_Char_Match);
	CPPUNIT_TEST(ReplaceFirst_Char_NoMatch);
	CPPUNIT_TEST(ReplaceLast_Char_Match);
	CPPUNIT_TEST(ReplaceLast_Char_NoMatch);
	CPPUNIT_TEST(ReplaceAll_Char_Match);
	CPPUNIT_TEST(ReplaceAll_Char_NoMatch);
	CPPUNIT_TEST(ReplaceAll_Char_Same);
	CPPUNIT_TEST(ReplaceAll_Char_WithOffset);
	CPPUNIT_TEST(Replace_Char_InRange);
	CPPUNIT_TEST(Replace_Char_NoMatchInRange);
	CPPUNIT_TEST(Replace_Char_EmptyString);
	CPPUNIT_TEST(ReplaceFirst_String_Match);
	CPPUNIT_TEST(ReplaceFirst_String_NoMatch);
	CPPUNIT_TEST(ReplaceFirst_String_Null);
	CPPUNIT_TEST(ReplaceLast_String_Match);
	CPPUNIT_TEST(ReplaceLast_String_NoMatch);
	CPPUNIT_TEST(ReplaceLast_String_Null);
	CPPUNIT_TEST(ReplaceAll_String_Match);
	CPPUNIT_TEST(ReplaceAll_String_NoMatch);
	CPPUNIT_TEST(ReplaceAll_String_Null);
	CPPUNIT_TEST(ReplaceAll_String_SubMatch);
	CPPUNIT_TEST(IReplaceAll_String_Match);
	CPPUNIT_TEST(IReplaceAll_String_Same);
	CPPUNIT_TEST(IReplaceFirst_Char_Match);
	CPPUNIT_TEST(IReplaceFirst_Char_NoMatch);
	CPPUNIT_TEST(IReplaceLast_Char_Match);
	CPPUNIT_TEST(IReplaceLast_Char_NoMatch);
	CPPUNIT_TEST(IReplaceAll_Char_Match);
	CPPUNIT_TEST(IReplaceAll_Char_Same);
	CPPUNIT_TEST(IReplaceAll_Char_NoMatch);
	CPPUNIT_TEST(IReplaceAll_Char_WithOffset);
	CPPUNIT_TEST(IReplace_Char_InRange);
	CPPUNIT_TEST(IReplace_Char_NoMatchInRange);
	CPPUNIT_TEST(IReplace_Char_EmptyString);
	CPPUNIT_TEST(IReplaceFirst_String_MatchIgnoreCase);
	CPPUNIT_TEST(IReplaceFirst_String_NoMatch);
	CPPUNIT_TEST(IReplaceFirst_String_Null);
#ifndef TEST_R5
	CPPUNIT_TEST(IReplaceLast_String_MatchIgnoreCase);
#endif
	CPPUNIT_TEST(IReplaceLast_String_NoMatch);
	CPPUNIT_TEST(IReplaceLast_String_Null);
	CPPUNIT_TEST(IReplaceAll_String_MatchIgnoreCase);
	CPPUNIT_TEST(IReplaceAll_String_NoMatch);
	CPPUNIT_TEST(IReplaceAll_String_MatchLengthy);
	CPPUNIT_TEST(IReplaceAll_String_Null);
	CPPUNIT_TEST(ReplaceSet_Char_Single);
	CPPUNIT_TEST(ReplaceSet_Char_Multiple);
	CPPUNIT_TEST(ReplaceSet_Char_Same);
#ifndef TEST_R5
	CPPUNIT_TEST(ReplaceSet_String_Match);
	CPPUNIT_TEST(ReplaceSet_String_Swap);
	CPPUNIT_TEST(ReplaceSet_String_Erase);
#endif
	CPPUNIT_TEST(ReplaceSet_String_Perf1);
	CPPUNIT_TEST(ReplaceSet_String_Perf2);
	CPPUNIT_TEST(ReplaceAll_String_Perf1);
	CPPUNIT_TEST(ReplaceAll_String_Perf2);
	CPPUNIT_TEST(ReplaceSet_String_Perf3);
	CPPUNIT_TEST_SUITE_END();

public:
	void ReplaceFirst_Char_Match()
	{
		BString str("test string");
		str.ReplaceFirst('t', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "best string"));
	}

	void ReplaceFirst_Char_NoMatch()
	{
		BString str("test string");
		str.ReplaceFirst('x', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void ReplaceLast_Char_Match()
	{
		BString str("test string");
		str.ReplaceLast('t', 'w');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test swring"));
	}

	void ReplaceLast_Char_NoMatch()
	{
		BString str("test string");
		str.ReplaceLast('x', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void ReplaceAll_Char_Match()
	{
		BString str("test string");
		str.ReplaceAll('t', 'i');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "iesi siring"));
	}

	void ReplaceAll_Char_NoMatch()
	{
		BString str("test string");
		str.ReplaceAll('x', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void ReplaceAll_Char_Same()
	{
		BString str("test string");
		str.ReplaceAll('t', 't');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void ReplaceAll_Char_WithOffset()
	{
		BString str("test string");
		str.ReplaceAll('t', 'i', 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "tesi siring"));
	}

	void Replace_Char_InRange()
	{
		BString str("she sells sea shells on the sea shore");
		str.Replace('s', 't', 4, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she tellt tea thells on the sea shore"));
	}

	void Replace_Char_NoMatchInRange()
	{
		BString str("she sells sea shells on the sea shore");
		str.Replace('s', 's', 4, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the sea shore"));
	}

	void Replace_Char_EmptyString()
	{
		BString str;
		str.Replace('s', 'x', 12, 32);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), ""));
	}

	void ReplaceFirst_String_Match()
	{
		BString str("she sells sea shells on the seashore");
		str.ReplaceFirst("sea", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells the shells on the seashore"));
	}

	void ReplaceFirst_String_NoMatch()
	{
		BString str("she sells sea shells on the seashore");
		str.ReplaceFirst("tex", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void ReplaceFirst_String_Null()
	{
		BString str("Error moving \"%name\"");
		str.ReplaceFirst("%name", NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "Error moving \"\""));
	}

	void ReplaceLast_String_Match()
	{
		BString str("she sells sea shells on the seashore");
		str.ReplaceLast("sea", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the theshore"));
	}

	void ReplaceLast_String_NoMatch()
	{
		BString str("she sells sea shells on the seashore");
		str.ReplaceLast("tex", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void ReplaceLast_String_Null()
	{
		BString str("she sells sea shells on the seashore");
		str.ReplaceLast("sea", NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the shore"));
	}

	void ReplaceAll_String_Match()
	{
		BString str("abc abc abc");
		str.ReplaceAll("ab", "abc");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "abcc abcc abcc"));
	}

	void ReplaceAll_String_NoMatch()
	{
		BString str("abc abc abc");
		str.ReplaceAll("abc", "abc");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "abc abc abc"));
	}

	void ReplaceAll_String_Null()
	{
		BString str("abc abc abc");
		str.ReplaceAll("abc", NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "  "));
	}

	void ReplaceAll_String_SubMatch()
	{
		BString str("she sells sea shells on the seashore");
		str.ReplaceAll("tex", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void IReplaceAll_String_Match()
	{
		BString str("she sells sea shells on the seashore");
		str.IReplaceAll("sea", "the", 11);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the theshore"));
	}

	void IReplaceAll_String_Same()
	{
		BString str("she sells sea shells on the seashore");
		str.IReplaceAll("sea", "sea", 11);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void IReplaceFirst_Char_Match()
	{
		BString str("test string");
		str.IReplaceFirst('t', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "best string"));
	}

	void IReplaceFirst_Char_NoMatch()
	{
		BString str("test string");
		str.IReplaceFirst('x', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void IReplaceLast_Char_Match()
	{
		BString str("test string");
		str.IReplaceLast('t', 'w');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test swring"));
	}

	void IReplaceLast_Char_NoMatch()
	{
		BString str("test string");
		str.IReplaceLast('x', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void IReplaceAll_Char_Match()
	{
		BString str("TEST string");
		str.IReplaceAll('t', 'i');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "iESi siring"));
	}

	void IReplaceAll_Char_Same()
	{
		BString str("TEST string");
		str.IReplaceAll('t', 'T');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "TEST sTring"));
	}

	void IReplaceAll_Char_NoMatch()
	{
		BString str("test string");
		str.IReplaceAll('x', 'b');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "test string"));
	}

	void IReplaceAll_Char_WithOffset()
	{
		BString str("TEST string");
		str.IReplaceAll('t', 'i', 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "TESi siring"));
	}

	void IReplace_Char_InRange()
	{
		BString str("She sells Sea shells on the sea shore");
		str.IReplace('s', 't', 4, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "She tellt tea thells on the sea shore"));
	}

	void IReplace_Char_NoMatchInRange()
	{
		BString str("She sells Sea shells on the sea shore");
		str.IReplace('s', 's', 4, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "She sells sea shells on the sea shore"));
	}

	void IReplace_Char_EmptyString()
	{
		BString str;
		str.IReplace('s', 'x', 12, 32);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), ""));
	}

	void IReplaceFirst_String_MatchIgnoreCase()
	{
		BString str("she sells SeA shells on the seashore");
		str.IReplaceFirst("sea", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells the shells on the seashore"));
	}

	void IReplaceFirst_String_NoMatch()
	{
		BString str("she sells sea shells on the seashore");
		str.IReplaceFirst("tex", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void IReplaceFirst_String_Null()
	{
		BString str("she sells SeA shells on the seashore");
		str.IReplaceFirst("sea ", NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells shells on the seashore"));
	}
#ifndef TEST_R5
	void IReplaceLast_String_MatchIgnoreCase()
	{
		BString str("she sells sea shells on the SEashore");
		str.IReplaceLast("sea", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the theshore"));
	}
#endif
	void IReplaceLast_String_NoMatch()
	{
		BString str("she sells sea shells on the seashore");
		str.IReplaceLast("tex", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void IReplaceLast_String_Null()
	{
		BString str("she sells sea shells on the SEashore");
		str.IReplaceLast("sea", NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the shore"));
	}

	void IReplaceAll_String_MatchIgnoreCase()
	{
		BString str("abc ABc aBc");
		str.IReplaceAll("ab", "abc");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "abcc abcc abcc"));
	}

	void IReplaceAll_String_NoMatch()
	{
		BString str("she sells sea shells on the seashore");
		str.IReplaceAll("tex", "the");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells sea shells on the seashore"));
	}

	void IReplaceAll_String_MatchLengthy()
	{
		BString str("she sells SeA shells on the sEashore");
		str.IReplaceAll("sea", "the", 11);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "she sells SeA shells on the theshore"));
	}

	void IReplaceAll_String_Null()
	{
		BString str("abc ABc aBc");
		str.IReplaceAll("ab", NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "c c c"));
	}

	void ReplaceSet_Char_Single()
	{
		BString str("abc abc abc");
		str.ReplaceSet("ab", 'x');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "xxc xxc xxc"));
	}

	void ReplaceSet_Char_Multiple()
	{
		BString str("abcabcabcbababc");
		str.ReplaceSet("abc", 'c');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "ccccccccccccccc"));
	}

	void ReplaceSet_Char_Same()
	{
		BString str("abcabcabcbababc");
		str.ReplaceSet("c", 'c');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "abcabcabcbababc"));
	}
#ifndef TEST_R5
	void ReplaceSet_String_Match()
	{
		BString str("abcd abcd abcd");
		str.ReplaceSet("abcd ", "");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), ""));
	}

	void ReplaceSet_String_Swap()
	{
		BString str("abcd abcd abcd");
		str.ReplaceSet("ad", "da");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "dabcda dabcda dabcda"));
	}

	void ReplaceSet_String_Erase()
	{
		BString str("abcd abcd abcd");
		str.ReplaceSet("ad", "");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "bc bc bc"));
	}
#endif
	void ReplaceSet_String_Perf1()
	{
		BString str;
		int32 sz = 1024 * 50;
		char* buf = str.LockBuffer(sz);
		memset(buf, 'x', sz);
		str.UnlockBuffer(sz);
		str.ReplaceSet("x", "y");
		CPPUNIT_ASSERT_EQUAL(sz, str.Length());
	}

	void ReplaceSet_String_Perf2()
	{
		BString str;
		int32 sz = 1024 * 50;
		char* buf = str.LockBuffer(sz);
		memset(buf, 'x', sz);
		str.UnlockBuffer(sz);
		str.ReplaceSet("x", "");
		CPPUNIT_ASSERT_EQUAL(0, str.Length());
	}

	void ReplaceAll_String_Perf1()
	{
		BString str;
		int32 sz = 1024 * 50;
		char* buf = str.LockBuffer(sz);
		memset(buf, 'x', sz);
		str.UnlockBuffer(sz);
		str.ReplaceAll("x", "y");
		CPPUNIT_ASSERT_EQUAL(sz, str.Length());
	}

	void ReplaceAll_String_Perf2()
	{
		BString str;
		int32 sz = 1024 * 50;
		char* buf = str.LockBuffer(sz);
		memset(buf, 'x', sz);
		str.UnlockBuffer(sz);
		str.ReplaceAll("xx", "y");
		CPPUNIT_ASSERT_EQUAL(sz / 2, str.Length());
	}

	void ReplaceSet_String_Perf3()
	{
		BString str;
		int32 sz = 1024 * 50;
		char* buf = str.LockBuffer(sz);
		memset(buf, 'x', sz);
		str.UnlockBuffer(sz);
		str.ReplaceSet("xx", "");
		CPPUNIT_ASSERT_EQUAL(0, str.Length());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringReplaceTest, getTestSuiteName());
