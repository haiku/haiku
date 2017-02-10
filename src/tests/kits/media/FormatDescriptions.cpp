/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "FormatDescriptions.h"

#include <MediaFormats.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


FormatDescriptionsTest::FormatDescriptionsTest()
{
}


FormatDescriptionsTest::~FormatDescriptionsTest()
{
}


void
FormatDescriptionsTest::TestCompare()
{
	media_format_description a;
	a.family = B_AVI_FORMAT_FAMILY;
	a.u.avi.codec = 'DIVX';

	media_format_description b;
	CPPUNIT_ASSERT(!(a == b));

	b.family = a.family;
	CPPUNIT_ASSERT(!(a == b));

	a.family = B_QUICKTIME_FORMAT_FAMILY;
	a.u.quicktime.vendor = 5;
	a.u.quicktime.codec = 5;

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

	a.family = B_MISC_FORMAT_FAMILY;
	a.u.misc.file_format = 5;
	a.u.misc.codec = 5;

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


/*static*/ void
FormatDescriptionsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("FormatDescriptionsTest");

	suite.addTest(new CppUnit::TestCaller<FormatDescriptionsTest>(
		"FormatDescriptionsTest::TestCompare",
		&FormatDescriptionsTest::TestCompare));

	parent.addTest("FormatDescriptionsTest", &suite);
}
