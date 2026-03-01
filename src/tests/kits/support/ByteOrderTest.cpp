/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <math.h>
#include <string.h>

#include <ByteOrder.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>


// ToDo: swap_int16() and friends don't really belong here as they are in
// libroot.so
// The tests might be messed up because of that, and don't test the real
// thing, as long as they don't run on Haiku itself.


class ByteOrderTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ByteOrderTest);

	CPPUNIT_TEST(Swap16_InputZero_RemainsZero);
	CPPUNIT_TEST(Swap16_InputAscending_SwapsBytes);
	CPPUNIT_TEST(Swap16_InputNegative_SwapsBytes);
	CPPUNIT_TEST(Swap16_InputMixed_SwapsBytes);

	CPPUNIT_TEST(Swap32_InputZero_RemainsZero);
	CPPUNIT_TEST(Swap32_InputAscending_SwapsBytes);
	CPPUNIT_TEST(Swap32_InputNegative_SwapsBytes);
	CPPUNIT_TEST(Swap32_InputMixed_SwapsBytes);

	CPPUNIT_TEST(Swap64_InputZero_RemainsZero);
	CPPUNIT_TEST(Swap64_InputAscending_SwapsBytes);
	CPPUNIT_TEST(Swap64_InputNegative_SwapsBytes);
	CPPUNIT_TEST(Swap64_InputMixed_SwapsBytes);

	CPPUNIT_TEST(SwapFloat_Roundtrip_ReturnsInput);
	CPPUNIT_TEST(SwapDouble_Roundtrip_ReturnsInput);

	CPPUNIT_TEST(SwapData_StringType_ReturnsBadValue);
	CPPUNIT_TEST(SwapData_Int32TypeInputWithZeroLength_ReturnsOK);
	CPPUNIT_TEST(SwapData_Int32TypeWithNullInputSwapAlways_ReturnsBadValue);
	CPPUNIT_TEST(SwapData_Int32TypeWithNullInputSwapEndiannessToHost_ReturnsOK);

	CPPUNIT_TEST(AlgorithmCheck);
	CPPUNIT_TEST(IsTypeSwapped);

	CPPUNIT_TEST_SUITE_END();

public:
	void Swap16_InputZero_RemainsZero()
	{
		uint16 input = 0;
		CPPUNIT_ASSERT_EQUAL(input, __swap_int16(input));
	}

	void Swap16_InputAscending_SwapsBytes()
	{
		int16 input = 0x1234;
		uint16 expected = 0x3412;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int16(input));
	}

	void Swap16_InputNegative_SwapsBytes()
	{
		int16 input = 0xfedc;
		uint16 expected = 0xdcfe;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int16(input));
	}

	void Swap16_InputMixed_SwapsBytes()
	{
		uint16 input = 0xfefd;
		uint16 expected = 0xfdfe;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int16(input));
	}

	void Swap32_InputZero_RemainsZero()
	{
		uint32 input = 0;
		CPPUNIT_ASSERT_EQUAL(input, __swap_int32(input));
	}

	void Swap32_InputAscending_SwapsBytes()
	{
		int32 input = 0x12345678;
		uint32 expected = 0x78563412;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int32(input));
	}

	void Swap32_InputNegative_SwapsBytes()
	{
		int32 input = 0xfedcba98;
		uint32 expected = 0x98badcfe;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int32(input));
	}

	void Swap32_InputMixed_SwapsBytes()
	{
		uint32 input = 0xfefdfcfb;
		uint32 expected = 0xfbfcfdfe;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int32(input));
	}

	void Swap64_InputZero_RemainsZero()
	{
		uint64 input = 0;
		CPPUNIT_ASSERT_EQUAL(input, __swap_int64(input));
	}

	void Swap64_InputAscending_SwapsBytes()
	{
		int64 input = 0x1234567890000000LL;
		uint64 expected = 0x0000009078563412LL;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int64(input));
	}

	void Swap64_InputNegative_SwapsBytes()
	{
		int64 input = 0xfedcba9876543210LL;
		uint64 expected = 0x1032547698badcfeLL;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int64(input));
	}

	void Swap64_InputMixed_SwapsBytes()
	{
		uint64 input = 0xfefdLL;
		uint64 expected = 0xfdfe000000000000LL;
		CPPUNIT_ASSERT_EQUAL(expected, __swap_int64(input));
	}

	void SwapFloat_Roundtrip_ReturnsInput()
	{
		const float kNumber = 1.125;
		const float kNaN = NAN;
		const float kInfinity = HUGE_VALF;

		const float roundtrip = __swap_float(__swap_float(kNaN));
		uint32 expectedNaN;
		uint32 dataNaN;
		memcpy(&dataNaN, &roundtrip, sizeof(float));
		memcpy(&expectedNaN, &kNaN, sizeof(float));

		CPPUNIT_ASSERT_EQUAL(kNumber, __swap_float(__swap_float(kNumber)));
		CPPUNIT_ASSERT_EQUAL(expectedNaN, dataNaN); // NaN == NaN as floats returns false
		CPPUNIT_ASSERT_EQUAL(kInfinity, __swap_float(__swap_float(kInfinity)));
	}

	void SwapDouble_Roundtrip_ReturnsInput()
	{
		const double kNumber = 1.125;
		const double kNaN = NAN;
		const double kInfinity = HUGE_VAL;

		const double roundtrip = __swap_double(__swap_double(kNaN));
		uint64 expectedNaN;
		uint64 dataNaN;
		memcpy(&dataNaN, &roundtrip, sizeof(double));
		memcpy(&expectedNaN, &kNaN, sizeof(double));

		CPPUNIT_ASSERT_EQUAL(kNumber, __swap_double(__swap_double(kNumber)));
		CPPUNIT_ASSERT_EQUAL(expectedNaN, dataNaN); // NaN == NaN as floats returns false
		CPPUNIT_ASSERT_EQUAL(kInfinity, __swap_double(__swap_double(kInfinity)));
	}

	void SwapData_StringType_ReturnsBadValue()
	{
		char str[4];
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, swap_data(B_STRING_TYPE, str, 4, B_SWAP_ALWAYS));
	}

	void SwapData_Int32TypeInputWithZeroLength_ReturnsOK()
	{
		int32 num32 = 0;
		CPPUNIT_ASSERT_EQUAL(B_OK, swap_data(B_INT32_TYPE, &num32, 0, B_SWAP_ALWAYS));
	}

	void SwapData_Int32TypeWithNullInputSwapAlways_ReturnsBadValue()
	{
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, swap_data(B_INT32_TYPE, NULL, 4, B_SWAP_ALWAYS));
	}

	void SwapData_Int32TypeWithNullInputSwapEndiannessToHost_ReturnsOK()
	{
#if B_HOST_IS_LENDIAN
		CPPUNIT_ASSERT_EQUAL(B_OK, swap_data(B_INT32_TYPE, NULL, 4, B_SWAP_HOST_TO_LENDIAN));
#else
		CPPUNIT_ASSERT_EQUAL(B_OK, swap_data(B_INT32_TYPE, NULL, 4, B_SWAP_HOST_TO_BENDIAN));
#endif
	}

	void AlgorithmCheck()
	{
#define TEST(type, source, target) \
	memcpy(target, source, sizeof(source)); \
	for (int32 i = 0; i < 4; i++) { \
		if (B_HOST_IS_LENDIAN) { \
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_LENDIAN); \
			CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
			swap_data(type, target, sizeof(target), B_SWAP_LENDIAN_TO_HOST); \
			CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
\
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_BENDIAN); \
			CPPUNIT_ASSERT(memcmp(target, source, sizeof(source)) != 0); \
			swap_data(type, target, sizeof(target), B_SWAP_BENDIAN_TO_HOST); \
			CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
		} else if (B_HOST_IS_BENDIAN) { \
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_BENDIAN); \
			CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
			swap_data(type, target, sizeof(target), B_SWAP_BENDIAN_TO_HOST); \
			CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
\
			swap_data(type, target, sizeof(target), B_SWAP_HOST_TO_LENDIAN); \
			CPPUNIT_ASSERT(memcmp(target, source, sizeof(source)) != 0); \
			swap_data(type, target, sizeof(target), B_SWAP_LENDIAN_TO_HOST); \
			CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
		} \
\
		swap_data(type, target, sizeof(target), B_SWAP_ALWAYS); \
		CPPUNIT_ASSERT(memcmp(target, source, sizeof(source)) != 0); \
		swap_data(type, target, sizeof(target), B_SWAP_ALWAYS); \
		CPPUNIT_ASSERT_EQUAL(0, memcmp(target, source, sizeof(source))); \
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

		const double kArrayDouble[] = {3.42, 0.0, NAN, HUGE_VAL};
		double arrayDouble[4];
		TEST(B_DOUBLE_TYPE, kArrayDouble, arrayDouble);

#undef TEST
	}

	void IsTypeSwapped()
	{
#define IS_SWAPPED(x) CPPUNIT_ASSERT_EQUAL(true, is_type_swapped(x))
#define NOT_SWAPPED(x) CPPUNIT_ASSERT_EQUAL(false, is_type_swapped(x))

		NOT_SWAPPED(B_ANY_TYPE);
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
		NOT_SWAPPED(B_OBJECT_TYPE);
		IS_SWAPPED(B_OFF_T_TYPE);
		IS_SWAPPED(B_PATTERN_TYPE);
		IS_SWAPPED(B_POINTER_TYPE);
		IS_SWAPPED(B_POINT_TYPE);
		NOT_SWAPPED(B_RAW_TYPE);
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
		NOT_SWAPPED(B_MEDIA_PARAMETER_TYPE);
		NOT_SWAPPED(B_MEDIA_PARAMETER_WEB_TYPE);
		NOT_SWAPPED(B_MEDIA_PARAMETER_GROUP_TYPE);
		NOT_SWAPPED(B_ASCII_TYPE);

		NOT_SWAPPED('    ');
		NOT_SWAPPED('0000');
		NOT_SWAPPED('1111');
		NOT_SWAPPED('aaaa');

#undef IS_SWAPPED
#undef NOT_SWAPPED
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ByteOrderTest, getTestSuiteName());
