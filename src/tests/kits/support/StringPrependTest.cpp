/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>
#include <UTF8.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringPrependTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringPrependTest);
  CPPUNIT_TEST(BString_PrependsString);
  CPPUNIT_TEST(CString_PrependsString);
  CPPUNIT_TEST(NullPointer_IgnoresPrepend);
  CPPUNIT_TEST(CStringAndLength_PrependsSubstring);
  CPPUNIT_TEST(BStringAndLength_PrependsSubstring);
  CPPUNIT_TEST(CharAndLength_PrependsMultipleChars);
  CPPUNIT_TEST(CStringToEmpty_PrependsString);
  CPPUNIT_TEST_SUITE_END();

public:
  void BString_PrependsString()
  {
  	BString str1("a String");
  	BString str2("PREPENDED");
  	str1.Prepend(str2);
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "PREPENDEDa String"));
  }

  void CString_PrependsString()
  {
  	BString str1("String");
  	str1.Prepend("PREPEND");
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "PREPENDString"));
  }

  void NullPointer_IgnoresPrepend()
  {
  	BString str1("String");
  	str1.Prepend((char*)NULL);
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "String"));
  }

  void CStringAndLength_PrependsSubstring()
  {
  	BString str1("String");
  	str1.Prepend("PREPENDED", 3);
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "PREString"));
  }

  void BStringAndLength_PrependsSubstring()
  {
  	BString str1("String");
  	BString str2("PREPEND", 4);
  	str1.Prepend(str2);
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "PREPString"));
  }

  void CharAndLength_PrependsMultipleChars()
  {
  	BString str1("aString");
  	str1.Prepend('c', 4);
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "ccccaString"));
  }

  void CStringToEmpty_PrependsString()
  {
  	BString str1;
  	str1.Prepend("PREPENDED");
  	CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "PREPENDED"));
  }
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringPrependTest, getTestSuiteName());
