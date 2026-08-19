/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#if defined(__i386__) || defined(__x86_64__)
#include <cpu_type.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class CpuTypeTests : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CpuTypeTests);
	CPPUNIT_TEST(NameWithInitialSpaces_ParseIntel_ReturnsNameWithoutSpaces);
	CPPUNIT_TEST(NameWithVendor_ParseIntel_ReturnsNameWithoutVendor);
	CPPUNIT_TEST(NameWithRegisteredMarkVendor_ParseIntel_ReturnsNameWithoutVendorAndMark);
	CPPUNIT_TEST(NameWithTextRAndTM_ParseIntel_ReplacesWithUTF8);
	CPPUNIT_TEST(NameWithCPUProcessor_ParseIntel_RemovesWords);
	CPPUNIT_TEST(NameWithAtRemainder_ParseIntel_RemovesRemainder);
	CPPUNIT_TEST(NameWithDuplicateSpacesInTheMiddle_ParseIntel_RemovesDuplicates);
	CPPUNIT_TEST(NameWithDuplicateSpacesAtTheEnd_ParseIntel_RemovesDuplicates);
	CPPUNIT_TEST(NameWithMHzWithoutAtCharBefore_ParseIntel_RemovesClockValue);
	CPPUNIT_TEST(NameWithGHzWithoutAtCharBefore_ParseIntel_RemovesClockValue);
	CPPUNIT_TEST(NameWithDecimalWithoutAtCharBefore_ParseIntel_RemovesClockValue);
	CPPUNIT_TEST(NameWithEverything_ParseIntel_ReturnsExpectedName);
	CPPUNIT_TEST_SUITE_END();

public:
	void NameWithInitialSpaces_ParseIntel_ReturnsNameWithoutSpaces()
	{
		const char* name = "     test";
		const char* expected = "test";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithVendor_ParseIntel_ReturnsNameWithoutVendor()
	{
		const char* name = "Intel Core 2";
		const char* expected = "Core 2";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithRegisteredMarkVendor_ParseIntel_ReturnsNameWithoutVendorAndMark()
	{
		const char* name = "Intel(R) Core 2";
		const char* expected = "Core 2";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithTextRAndTM_ParseIntel_ReplacesWithUTF8()
	{
		const char* name = "Core(R) Centrino(TM) Ultra";
		const char* expected = "Core® Centrino™ Ultra";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithCPUProcessor_ParseIntel_RemovesWords()
	{
		const char* name = "Core CPU 2 processor";
		const char* expected = "Core 2";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithAtRemainder_ParseIntel_RemovesRemainder()
	{
		const char* name = "Core 2 @ 3 GHz per core";
		const char* expected = "Core 2";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithDuplicateSpacesInTheMiddle_ParseIntel_RemovesDuplicates()
	{
		const char* name = "Core        2   Duo";
		const char* expected = "Core 2 Duo";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithDuplicateSpacesAtTheEnd_ParseIntel_RemovesDuplicates()
	{
		const char* name = "Core 2      ";
		const char* expected = "Core 2";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithMHzWithoutAtCharBefore_ParseIntel_RemovesClockValue()
	{
		const char* name = "Celeron M 900MHz";
		const char* expected = "Celeron M";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithGHzWithoutAtCharBefore_ParseIntel_RemovesClockValue()
	{
		const char* name = "Pentium D 2GHz";
		const char* expected = "Pentium D";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithDecimalWithoutAtCharBefore_ParseIntel_RemovesClockValue()
	{
		const char* name = "Celeron M 1.5GHz";
		const char* expected = "Celeron M";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}

	void NameWithEverything_ParseIntel_ReturnsExpectedName()
	{
		const char* name = "Intel(R) Celeron(R) CPU Core(TM)      1500MHz @ 1.5GHz";
		const char* expected = "Celeron® Core™";
		const char* result = parse_intel(name);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(expected, result));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CpuTypeTests, getTestSuiteName());
#endif
