/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ByteOrderTest.h"

#include <math.h>
#include <string.h>

#include <ByteOrder.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <TestUtils.h>


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
	const double kInfinity = HUGE_VAL;

	CHK(kNumber == __swap_double(__swap_double(kNumber)));
	CHK(kNaN == __swap_double(__swap_double(kNaN)));
	CHK(kInfinity == __swap_double(__swap_double(kInfinity)));
}


//	#pragma mark -


class SwapDataTest : public BTestCase {
	public:
		SwapDataTest(std::string name = "");

		static Test *suite(void);
		void test(void);
};


SwapDataTest::SwapDataTest(std::string name)
	: BTestCase(name)
{
}


Test *
SwapDataTest::suite(void)
{
	return new CppUnit::TestCaller<SwapDataTest>("ByteOrderTest::SwapDataTest", &SwapDataTest::test);
}


void 
SwapDataTest::test(void)
{
	// error checking
	char string[4];
	CHK(swap_data(B_STRING_TYPE, string, 4, B_SWAP_ALWAYS) == B_BAD_VALUE);
	int32 num32 = 0;
	CHK(swap_data(B_INT32_TYPE, &num32, 0, B_SWAP_ALWAYS) == B_BAD_VALUE);
	CHK(swap_data(B_INT32_TYPE, NULL, 4, B_SWAP_ALWAYS) == B_BAD_VALUE);
#if B_HOST_IS_LENDIAN
	CHK(swap_data(B_INT32_TYPE, NULL, 4, B_SWAP_HOST_TO_LENDIAN) == B_BAD_VALUE);
#else
	CHK(swap_data(B_INT32_TYPE, NULL, 4, B_SWAP_HOST_TO_BENDIAN) == B_BAD_VALUE);
#endif

	// algorithm checking
#define TEST(type, source, target) \
	memcpy(target, source, sizeof(source)); \
	for (int32 i = 0; i < 4; i++) { \
		if (B_HOST_IS_LENDIAN) { \
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_LENDIAN); \
			CHK(!memcmp(target, source, sizeof(source))); \
			swap_data(type, target, sizeof(target), B_SWAP_LENDIAN_TO_HOST); \
			CHK(!memcmp(target, source, sizeof(source))); \
			\
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_BENDIAN); \
			CHK(memcmp(target, source, sizeof(source))); \
			swap_data(type, target, sizeof(target), B_SWAP_BENDIAN_TO_HOST); \
			CHK(!memcmp(target, source, sizeof(source))); \
		} else if (B_HOST_IS_BENDIAN) { \
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_BENDIAN); \
			CHK(!memcmp(target, source, sizeof(source))); \
			swap_data(type, target, sizeof(target), B_SWAP_BENDIAN_TO_HOST); \
			CHK(!memcmp(target, source, sizeof(source))); \
			\
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_LENDIAN); \
			CHK(memcmp(target, source, sizeof(source))); \
			swap_data(type, target, sizeof(target), B_SWAP_LENDIAN_TO_HOST); \
			CHK(!memcmp(target, source, sizeof(source))); \
		} \
		\
		swap_data(type, target, sizeof(target), B_SWAP_ALWAYS); \
		CHK(memcmp(target, source, sizeof(source))); \
		swap_data(type, target, sizeof(target), B_SWAP_ALWAYS); \
		CHK(!memcmp(target, source, sizeof(source))); \
	}

	const uint64 kArray64[] = {0x0123456789abcdefULL, 0x1234, 0x5678000000000000ULL, 0x0};
	uint64 array64[4];
	TEST(B_UINT64_TYPE, kArray64, array64);

	const uint32 kArray32[] = {0x12345678, 0x1234, 0x56780000, 0x0};
	uint32 array32[4];
	TEST(B_UINT32_TYPE, kArray32, array32);

	const uint16 kArray16[] = {0x1234, 0x12, 0x3400, 0x0};
	uint16 array16[4];
	TEST(B_UINT16_TYPE, kArray16, array16);

	const float kArrayFloat[] = {3.4f, 0.0f, NAN, HUGE_VALF};
	float arrayFloat[4];
	TEST(B_FLOAT_TYPE, kArrayFloat, arrayFloat);

	const float kArrayDouble[] = {3.42, 0.0, NAN, HUGE_VAL};
	double arrayDouble[4];
	TEST(B_DOUBLE_TYPE, kArrayDouble, arrayDouble);

#undef TEST
}


//	#pragma mark -


class IsTypeSwappedTest : public BTestCase {
	public:
		IsTypeSwappedTest(std::string name = "");

		static Test *suite(void);
		void test(void);
};


IsTypeSwappedTest::IsTypeSwappedTest(std::string name)
	: BTestCase(name)
{
}


Test *
IsTypeSwappedTest::suite(void)
{
	return new CppUnit::TestCaller<IsTypeSwappedTest>("ByteOrderTest::IsTypeSwappedTest", &IsTypeSwappedTest::test);
}


void 
IsTypeSwappedTest::test(void)
{
#define IS_SWAPPED(x) CHK(is_type_swapped(x))
#define NOT_SWAPPED(x) CHK(!is_type_swapped(x))

	IS_SWAPPED(B_ANY_TYPE);
	IS_SWAPPED(B_BOOL_TYPE);
	IS_SWAPPED(B_CHAR_TYPE);
	IS_SWAPPED(B_COLOR_8_BIT_TYPE);
	IS_SWAPPED(B_DOUBLE_TYPE);
	IS_SWAPPED(B_FLOAT_TYPE);
	IS_SWAPPED(B_GRAYSCALE_8_BIT_TYPE);
	IS_SWAPPED(B_INT64_TYPE);
	IS_SWAPPED(B_INT32_TYPE);
	IS_SWAPPED(B_INT16_TYPE);
	IS_SWAPPED(B_INT8_TYPE);
	IS_SWAPPED(B_MESSAGE_TYPE);
	IS_SWAPPED(B_MESSENGER_TYPE);
	IS_SWAPPED(B_MIME_TYPE);
	IS_SWAPPED(B_MONOCHROME_1_BIT_TYPE);
	IS_SWAPPED(B_OBJECT_TYPE);
	IS_SWAPPED(B_OFF_T_TYPE);
	IS_SWAPPED(B_PATTERN_TYPE);
	IS_SWAPPED(B_POINTER_TYPE);
	IS_SWAPPED(B_POINT_TYPE);
	IS_SWAPPED(B_RAW_TYPE);
	IS_SWAPPED(B_RECT_TYPE);
	IS_SWAPPED(B_REF_TYPE);
	IS_SWAPPED(B_RGB_32_BIT_TYPE);
	IS_SWAPPED(B_RGB_COLOR_TYPE);
	IS_SWAPPED(B_SIZE_T_TYPE);
	IS_SWAPPED(B_SSIZE_T_TYPE);
	IS_SWAPPED(B_STRING_TYPE);
	IS_SWAPPED(B_TIME_TYPE);
	IS_SWAPPED(B_UINT64_TYPE);
	IS_SWAPPED(B_UINT32_TYPE);
	IS_SWAPPED(B_UINT16_TYPE);
	IS_SWAPPED(B_UINT8_TYPE);
	IS_SWAPPED(B_MEDIA_PARAMETER_TYPE);
	IS_SWAPPED(B_MEDIA_PARAMETER_WEB_TYPE);
	IS_SWAPPED(B_MEDIA_PARAMETER_GROUP_TYPE);
	IS_SWAPPED(B_ASCII_TYPE);

	NOT_SWAPPED('    ');
	NOT_SWAPPED('0000');
	NOT_SWAPPED('1111');
	NOT_SWAPPED('aaaa');

#undef IS_SWAPPED
#undef NOT_SWAPPED
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
	testSuite->addTest(new SwapDataTest("swap_data()"));
	testSuite->addTest(new IsTypeSwappedTest("is_type_swapped()"));

	return testSuite;
}







