/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "common.h"
#include "GraphicsDefsTest.h"
#include "TestCase.h"
#include <TestUtils.h>

#include <GraphicsDefs.h>


// patterns
const pattern _B_SOLID_HIGH = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const pattern _B_MIXED_COLORS = {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}};
const pattern _B_SOLID_LOW = {{0, 0, 0, 0, 0, 0, 0, 0}};

// colors
const rgb_color	_B_TRANSPARENT_COLOR = {0x77, 0x74, 0x77, 0x00};
const rgb_color	_B_TRANSPARENT_32_BIT = {0x77, 0x74, 0x77, 0x00};
const uint8 _B_TRANSPARENT_8_BIT = 0xff;

const uint8 _B_TRANSPARENT_MAGIC_CMAP8 = 0xff;
const uint16 _B_TRANSPARENT_MAGIC_RGBA15 = 0x39ce;
const uint16 _B_TRANSPARENT_MAGIC_RGBA15_BIG = 0xce39;
const uint32 _B_TRANSPARENT_MAGIC_RGBA32 = 0x00777477;
const uint32 _B_TRANSPARENT_MAGIC_RGBA32_BIG = 0x77747700;


// misc.
const struct screen_id _B_MAIN_SCREEN_ID = {0};

template<class T> void compare(T &a, T &b);


template<>
void
compare<const pattern>(const pattern &a, const pattern &b)
{
	for (int32 i = 0; i < 8; i++)
		CHK(a.data[i] == b.data[i]);
}


template<>
void
compare<const rgb_color>(const rgb_color &a, const rgb_color &b)
{
	CHK(a.red == b.red);
	CHK(a.green == b.green);
	CHK(a.blue == b.blue);
	CHK(a.alpha == b.alpha);
}


template<class T>
void
compare(T &a, T &b)
{
	CHK(a == b);
}


class ConstantsTest : public BTestCase {
	public:
		ConstantsTest(std::string name = "");

		static Test *suite(void);
		void test(void);
};


ConstantsTest::ConstantsTest(std::string name)
	: BTestCase(name)
{
}


Test *
ConstantsTest::suite(void)
{
	return new CppUnit::TestCaller<ConstantsTest>("GraphicsDefs::Constants", &ConstantsTest::test);
}


void 
ConstantsTest::test(void)
{
#define TEST(type, constant) compare(_##constant, constant)

	TEST(pattern, B_SOLID_LOW);
	TEST(pattern, B_MIXED_COLORS);
	TEST(pattern, B_SOLID_HIGH);

	TEST(rgb_color, B_TRANSPARENT_COLOR);
	TEST(rgb_color, B_TRANSPARENT_32_BIT);
	TEST(uint8, B_TRANSPARENT_MAGIC_CMAP8);
	TEST(uint16, B_TRANSPARENT_MAGIC_RGBA15);
	TEST(uint16, B_TRANSPARENT_MAGIC_RGBA15_BIG);
	TEST(uint32, B_TRANSPARENT_MAGIC_RGBA32);
	TEST(uint32, B_TRANSPARENT_MAGIC_RGBA32_BIG);
	TEST(uint8, B_TRANSPARENT_8_BIT);

	TEST(uint32, B_MAIN_SCREEN_ID.id);

#undef TEST
}


//	#pragma mark -


Test *
GraphicsDefsTestSuite()
{
	TestSuite *testSuite = new TestSuite();

	testSuite->addTest(new ConstantsTest("Constants"));

	return testSuite;
}







