/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "TestCase.h"
#include "ByteOrderTest.h"
#include <TestUtils.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <ByteOrder.h>

#include <math.h>


using namespace CppUnit;


// ToDo: swap_int16() and friends don't really belong here as they are in libroot.so
//	The tests might be messed up because of that, and don't test the real thing, as
//	long as they don't run on Haiku itself.


class Swap16Test : public BTestCase {
	public:
		Swap16Test(std::string name = "");

		static Test *suite(void);
		void test(void);
};


Swap16Test::Swap16Test(std::string name)
	: BTestCase(name)
{
}


Test *
Swap16Test::suite(void)
{
	return new CppUnit::TestCaller<Swap16Test>("ByteOrderTest::Swap16Test", &Swap16Test::test);
}


void 
Swap16Test::test(void)
{
	static int16 kNull = 0;
	static int16 kAscending = 0x1234;
	static int16 kAscendingSwapped = 0x3412;
	static int16 kNegative = 0xfedc;
	static int16 kNegativeSwapped = 0xdcfe;
	static uint16 kMix = 0xfefd;
	static uint16 kMixSwapped = 0xfdfe;

	CHK(kNull == __swap_int16(kNull));
	CHK(kAscendingSwapped == __swap_int16(kAscending));
	CHK(kNegativeSwapped == __swap_int16(kNegative));
	CHK(kMixSwapped == __swap_int16(kMix));
}


//	#pragma mark -


class Swap32Test : public BTestCase {
	public:
		Swap32Test(std::string name = "");

		static Test *suite(void);
		void test(void);
};


Swap32Test::Swap32Test(std::string name)
	: BTestCase(name)
{
}


Test *
Swap32Test::suite(void)
{
	return new CppUnit::TestCaller<Swap32Test>("ByteOrderTest::Swap32Test", &Swap32Test::test);
}


void 
Swap32Test::test(void)
{
	static int32 kNull = 0;
	static int32 kAscending = 0x12345678;
	static int32 kAscendingSwapped = 0x78563412;
	static int32 kNegative = 0xfedcba98;
	static int32 kNegativeSwapped = 0x98badcfe;
	static uint32 kMix = 0xfefdfcfb;
	static uint32 kMixSwapped = 0xfbfcfdfe;

	CHK((uint32)kNull == __swap_int32(kNull));
	CHK((uint32)kAscendingSwapped == __swap_int32(kAscending));
	CHK((uint32)kNegativeSwapped == __swap_int32(kNegative));
	CHK(kMixSwapped == __swap_int32(kMix));
}


//	#pragma mark -


class Swap64Test : public BTestCase {
	public:
		Swap64Test(std::string name = "");

		static Test *suite(void);
		void test(void);
};


Swap64Test::Swap64Test(std::string name)
	: BTestCase(name)
{
}


Test *
Swap64Test::suite(void)
{
	return new CppUnit::TestCaller<Swap64Test>("ByteOrderTest::Swap64Test", &Swap64Test::test);
}


void 
Swap64Test::test(void)
{
	static int64 kNull = 0LL;
	static int64 kAscending = 0x1234567890LL;
	static int64 kAscendingSwapped = 0x0000009078563412LL;
	static int64 kNegative = 0xfedcba9876543210LL;
	static int64 kNegativeSwapped = 0x1032547698badcfeLL;
	static uint64 kMix = 0xfefdLL;
	static uint64 kMixSwapped = 0xfdfe000000000000LL;

	CHK((uint64)kNull == __swap_int64(kNull));
	CHK((uint64)kAscendingSwapped == __swap_int64(kAscending));
	CHK((uint64)kNegativeSwapped == __swap_int64(kNegative));
	CHK(kMixSwapped == __swap_int64(kMix));
}


//	#pragma mark -


class SwapFloatTest : public BTestCase {
	public:
		SwapFloatTest(std::string name = "");

		static Test *suite(void);
		void test(void);
};


SwapFloatTest::SwapFloatTest(std::string name)
	: BTestCase(name)
{
}


Test *
SwapFloatTest::suite(void)
{
	return new CppUnit::TestCaller<SwapFloatTest>("ByteOrderTest::SwapFloatTest", &SwapFloatTest::test);
}


void 
SwapFloatTest::test(void)
{
	const float kNumber = 1.125;
	const float kNaN = NAN;
	const float kInfinity = HUGE_VALF;

	CHK(kNumber == __swap_float(__swap_float(kNumber)));
	CHK(kNaN == __swap_float(__swap_float(kNaN)));
	CHK(kInfinity == __swap_float(__swap_float(kInfinity)));
}


//	#pragma mark -


class SwapDoubleTest : public BTestCase {
	public:
		SwapDoubleTest(std::string name = "");

		static Test *suite(void);
		void test(void);
};


SwapDoubleTest::SwapDoubleTest(std::string name)
	: BTestCase(name)
{
}


Test *
SwapDoubleTest::suite(void)
{
	return new CppUnit::TestCaller<SwapDoubleTest>("ByteOrderTest::SwapDoubleTest", &SwapDoubleTest::test);
}


void 
SwapDoubleTest::test(void)
{
	const double kNumber = 1.125;
	const double kNaN = NAN;
	const double kInfinity = HUGE_VALF;

	CHK(kNumber == __swap_double(__swap_double(kNumber)));
	CHK(kNaN == __swap_double(__swap_double(kNaN)));
	CHK(kInfinity == __swap_double(__swap_double(kInfinity)));
}


//	#pragma mark -


Test *
ByteOrderTestSuite()
{
	TestSuite *testSuite = new TestSuite();

	testSuite->addTest(new Swap16Test("__swap_int16()"));
	testSuite->addTest(new Swap32Test("__swap_int32()"));
	testSuite->addTest(new Swap64Test("__swap_int64()"));
	testSuite->addTest(new SwapFloatTest("__swap_float()"));
	testSuite->addTest(new SwapDoubleTest("__swap_double()"));
//	testSuite->addTest(new SwapDataTest("swap_data()"));
//	testSuite->addTest(new IsTypeSwappedTest("is_type_swapped()"));

	return testSuite;
}







