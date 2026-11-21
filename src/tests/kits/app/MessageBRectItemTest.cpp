/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <Rect.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct BRectInitPolicy : public ArrayTypeBase<BRect> {
	static BRect Zero() { return BRect(0, 0, 0, 0); }
	static BRect Test1() { return BRect(1, 2, 3, 4); }
	static BRect Test2() { return BRect(5, 6, 7, 8); }
	static size_t SizeOf(const BRect&) { return sizeof(BRect); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(BRect(1, 2, 3, 4));
		array.push_back(BRect(4, 5, 6, 7));
		array.push_back(BRect(7, 8, 9, 10));
		return array;
	}
};


struct BRectAssertPolicy {
	static BRect Zero() { return BRect(0, 0, 0, 0); }
	static BRect Invalid() { return BRect(0, 0, -1, -1); }
	static bool Size(size_t size, BRect& r) { return size == sizeof(r); }
};


typedef
MessageItemFuncPolicy<BRect, &BMessage::AddRect, &BMessage::FindRect,
		&BMessage::FindRect, &BMessage::FindRect, &BMessage::HasRect,
		&BMessage::ReplaceRect>
	BRectFuncPolicy;
typedef
MessageItemTest<BRect, B_RECT_TYPE, BRectFuncPolicy, BRectInitPolicy,
		BRectAssertPolicy>
	MessageBRectItemTest;


std::ostream&
operator<<(std::ostream& os, const BRect& rect)
{
	int precision = os.precision();
	os.precision(1);
	os << "rect"
	   << "(l:" << rect.left << " t:" << rect.top << " r:" << rect.right
	   << " b:" << rect.bottom << ")";
	os.precision(precision);

	return os;
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageBRectItemTest, getTestSuiteName());
