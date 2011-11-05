#include "StringReplaceTest.h"
#include "cppunit/TestCaller.h"
#include <String.h>

StringReplaceTest::StringReplaceTest(std::string name) :
		BTestCase(name)
{
}



StringReplaceTest::~StringReplaceTest()
{
}


void
StringReplaceTest::PerformTest(void)
{
	BString *str1;
	const int32 sz = 1024*50;
	char* buf;

	//&ReplaceFirst(char, char);
	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceFirst('t', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "best string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceFirst('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&ReplaceLast(char, char);
	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceLast('t', 'w');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test swring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceLast('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&ReplaceAll(char, char, int32);
	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('t', 'i');
	CPPUNIT_ASSERT(strcmp(str1->String(), "iesi siring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('t', 't');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->ReplaceAll('t', 'i', 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "tesi siring") == 0);
	delete str1;

	//&Replace(char, char, int32, int32)
	NextSubTest();
	str1 = new BString("she sells sea shells on the sea shore");
	str1->Replace('s', 't', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "she tellt tea thells on the sea shore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the sea shore");
	str1->Replace('s', 's', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "she sells sea shells on the sea shore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString();
	str1->Replace('s', 'x', 12, 32);
	CPPUNIT_ASSERT(strcmp(str1->String(), "") == 0);
	delete str1;

	//&ReplaceFirst(const char*, const char*)
	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->ReplaceFirst("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells the shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->ReplaceFirst("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&ReplaceLast(const char*, const char*)
	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->ReplaceLast("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the theshore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->ReplaceLast("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&ReplaceAll(const char*, const char*, int32)
	NextSubTest();
	str1 = new BString("abc abc abc");
	str1->ReplaceAll("ab", "abc");
	CPPUNIT_ASSERT(strcmp(str1->String(), "abcc abcc abcc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("abc abc abc");
	str1->ReplaceAll("abc", "abc");
	CPPUNIT_ASSERT(strcmp(str1->String(), "abc abc abc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->ReplaceAll("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->IReplaceAll("sea", "the", 11);
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the theshore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->IReplaceAll("sea", "sea", 11);
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&IReplaceFirst(char, char);
	NextSubTest();
	str1 = new BString("test string");
	str1->IReplaceFirst('t', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "best string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->IReplaceFirst('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&IReplaceLast(char, char);
	NextSubTest();
	str1 = new BString("test string");
	str1->IReplaceLast('t', 'w');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test swring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->IReplaceLast('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&IReplaceAll(char, char, int32);
	NextSubTest();
	str1 = new BString("TEST string");
	str1->IReplaceAll('t', 'i');
	CPPUNIT_ASSERT(strcmp(str1->String(), "iESi siring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("TEST string");
	str1->IReplaceAll('t', 'T');
	CPPUNIT_ASSERT(strcmp(str1->String(), "TEST sTring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("test string");
	str1->IReplaceAll('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("TEST string");
	str1->IReplaceAll('t', 'i', 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "TESi siring") == 0);
	delete str1;

	//&IReplace(char, char, int32, int32)
	NextSubTest();
	str1 = new BString("She sells Sea shells on the sea shore");
	str1->IReplace('s', 't', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "She tellt tea thells on the sea shore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("She sells Sea shells on the sea shore");
	str1->IReplace('s', 's', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "She sells sea shells on the sea shore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString();
	str1->IReplace('s', 'x', 12, 32);
	CPPUNIT_ASSERT(strcmp(str1->String(), "") == 0);
	delete str1;

	//&IReplaceFirst(const char*, const char*)
	NextSubTest();
	str1 = new BString("she sells SeA shells on the seashore");
	str1->IReplaceFirst("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells the shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->IReplaceFirst("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&IReplaceLast(const char*, const char*)
#ifndef TEST_R5
	NextSubTest();
	str1 = new BString("she sells sea shells on the SEashore");
	str1->IReplaceLast("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the theshore") == 0);
	delete str1;
#endif
	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->IReplaceLast("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&IReplaceAll(const char*, const char*, int32)
	NextSubTest();
	str1 = new BString("abc ABc aBc");
	str1->IReplaceAll("ab", "abc");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"abcc abcc abcc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells sea shells on the seashore");
	str1->IReplaceAll("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("she sells SeA shells on the sEashore");
	str1->IReplaceAll("sea", "the", 11);
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells SeA shells on the theshore") == 0);
	delete str1;

	//ReplaceSet(const char*, char)
	NextSubTest();
	str1 = new BString("abc abc abc");
	str1->ReplaceSet("ab", 'x');
	CPPUNIT_ASSERT(strcmp(str1->String(), "xxc xxc xxc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("abcabcabcbababc");
	str1->ReplaceSet("abc", 'c');
	CPPUNIT_ASSERT(strcmp(str1->String(), "ccccccccccccccc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BString("abcabcabcbababc");
	str1->ReplaceSet("c", 'c');
	CPPUNIT_ASSERT(strcmp(str1->String(), "abcabcabcbababc") == 0);
	delete str1;

#ifndef TEST_R5
	//ReplaceSet(const char*, const char*)
	NextSubTest();
	str1 = new BString("abcd abcd abcd");
	str1->ReplaceSet("abcd ", "");
	CPPUNIT_ASSERT(strcmp(str1->String(), "") == 0);
	delete str1;
#endif

#ifndef TEST_R5
	//ReplaceSet(const char*, const char*)
	NextSubTest();
	str1 = new BString("abcd abcd abcd");
	str1->ReplaceSet("ad", "da");
	CPPUNIT_ASSERT(strcmp(str1->String(), "dabcda dabcda dabcda") == 0);
	delete str1;
#endif

#ifndef TEST_R5
	//ReplaceSet(const char*, const char*)
	NextSubTest();
	str1 = new BString("abcd abcd abcd");
	str1->ReplaceSet("ad", "");
	CPPUNIT_ASSERT(strcmp(str1->String(), "bc bc bc") == 0);
	delete str1;
#endif

	// we repeat some test, but this time with a bit of data
	// to test the performance:

	// ReplaceSet(const char*, const char*)
	NextSubTest();
	str1 = new BString();
	buf = str1->LockBuffer(sz);
	memset( buf, 'x', sz);
	str1->UnlockBuffer( sz);
	str1->ReplaceSet("x", "y");
	CPPUNIT_ASSERT(str1->Length() == sz);
	delete str1;

	NextSubTest();
	str1 = new BString();
	buf = str1->LockBuffer(sz);
	memset( buf, 'x', sz);
	str1->UnlockBuffer( sz);
	str1->ReplaceSet("x", "");
	CPPUNIT_ASSERT(str1->Length() == 0);
	delete str1;

	// ReplaceAll(const char*, const char*)
	NextSubTest();
	str1 = new BString();
	buf = str1->LockBuffer(sz);
	memset( buf, 'x', sz);
	str1->UnlockBuffer( sz);
	str1->ReplaceAll("x", "y");
	CPPUNIT_ASSERT(str1->Length() == sz);
	delete str1;

	NextSubTest();
	str1 = new BString();
	buf = str1->LockBuffer(sz);
	memset( buf, 'x', sz);
	str1->UnlockBuffer( sz);
	str1->ReplaceAll("xx", "y");
	CPPUNIT_ASSERT(str1->Length() == sz/2);
	delete str1;

	NextSubTest();
	str1 = new BString();
	buf = str1->LockBuffer(sz);
	memset( buf, 'x', sz);
	str1->UnlockBuffer( sz);
	str1->ReplaceSet("xx", "");
	CPPUNIT_ASSERT(str1->Length() == 0);
	delete str1;

}


CppUnit::Test *StringReplaceTest::suite(void)
{
	typedef CppUnit::TestCaller<StringReplaceTest>
		StringReplaceTestCaller;

	return(new StringReplaceTestCaller("BString::Replace Test", &StringReplaceTest::PerformTest));
}
