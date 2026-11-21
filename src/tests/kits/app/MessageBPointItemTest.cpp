/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <Point.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct PointInitPolicy : public ArrayTypeBase<BPoint> {
	static BPoint Zero() { return BPoint(0.0, 0.0); }
	static BPoint Test1() { return BPoint(10.0, 10.0); }
	static BPoint Test2() { return BPoint(20.0, 20.0); }
	static size_t SizeOf(const BPoint&) { return sizeof(BPoint); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(BPoint(30.0, 30.0));
		array.push_back(BPoint(40.0, 40.0));
		array.push_back(BPoint(50.0, 50.0));
		return array;
	}
};


struct PointAssertPolicy {
	static BPoint Zero() { return BPoint(0.0, 0.0); }
	static BPoint Invalid() { return BPoint(0.0, 0.0); }
	static bool Size(size_t size, BPoint& p) { return size == sizeof(p); }
};

typedef
MessageItemFuncPolicy<BPoint, &BMessage::AddPoint, &BMessage::FindPoint,
		&BMessage::FindPoint, &BMessage::FindPoint, &BMessage::HasPoint,
		&BMessage::ReplacePoint>
	PointFuncPolicy;
typedef
MessageItemTest<BPoint, B_POINT_TYPE, PointFuncPolicy, PointInitPolicy,
		PointAssertPolicy>
	MessageBPointItemTest;


std::ostream&
operator<<(std::ostream& os, const BPoint& point)
{
	int precision = os.precision();
	os << "point(x:" << point.x << ", y:" << point.y;
	os.precision(precision);
	return os;
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageBPointItemTest, getTestSuiteName());
