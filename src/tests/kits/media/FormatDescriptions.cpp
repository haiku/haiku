/*
 * Copyright 2004-2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MediaFormats.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class FormatDescriptionsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(FormatDescriptionsTest);
	CPPUNIT_TEST(MiscFamily_OperatorLess_ReturnsExpected);
	CPPUNIT_TEST(QuickTimeFamily_OperatorLess_ReturnsExpected);
	CPPUNIT_TEST(DifferentFamily_OperatorEqual_ReturnsFalse);
	CPPUNIT_TEST_SUITE_END();

public:
	void MiscFamily_OperatorLess_ReturnsExpected()
	{
		media_format_description a;
		a.family = B_MISC_FORMAT_FAMILY;
		a.u.misc.file_format = 5;
		a.u.misc.codec = 5;

		media_format_description b;
		b.family = B_MISC_FORMAT_FAMILY;
		b.u.misc.file_format = 6;
		b.u.misc.codec = 5;
		CPPUNIT_ASSERT(a < b);

		b.u.misc.file_format = 4;
		CPPUNIT_ASSERT(b < a);

		b.u.misc.file_format = 5;
		b.u.misc.codec = 6;
		CPPUNIT_ASSERT(a < b);

		b.u.misc.codec = 4;
		CPPUNIT_ASSERT(b < a);
	}

	void QuickTimeFamily_OperatorLess_ReturnsExpected()
	{
		media_format_description a;
		a.family = B_QUICKTIME_FORMAT_FAMILY;
		a.u.quicktime.vendor = 5;
		a.u.quicktime.codec = 5;

		media_format_description b;
		b.family = B_QUICKTIME_FORMAT_FAMILY;
		b.u.quicktime.vendor = 6;
		b.u.quicktime.codec = 5;
		CPPUNIT_ASSERT(a < b);

		b.u.quicktime.vendor = 4;
		CPPUNIT_ASSERT(b < a);

		b.u.quicktime.vendor = 5;
		b.u.quicktime.codec = 6;
		CPPUNIT_ASSERT(a < b);

		b.u.quicktime.codec = 4;
		CPPUNIT_ASSERT(b < a);
	}

	void DifferentFamily_OperatorEqual_ReturnsFalse()
	{
		media_format_description a;
		a.family = B_AVI_FORMAT_FAMILY;
		a.u.avi.codec = 'DIVX';

		media_format_description b;
		CPPUNIT_ASSERT(!(a == b));

		b.family = a.family;
		CPPUNIT_ASSERT(!(a == b));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FormatDescriptionsTest, getTestSuiteName());
