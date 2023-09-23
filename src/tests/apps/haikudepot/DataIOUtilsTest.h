/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_IO_UTILS_TEST_H
#define DATA_IO_UTILS_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class DataIOUtilsTest : public CppUnit::TestCase {
public:
								DataIOUtilsTest();
	virtual						~DataIOUtilsTest();

			void				TestReadBase64JwtClaims_1();
			void				TestReadBase64JwtClaims_2();
			void				TestCorrupt();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// DATA_IO_UTILS_TEST_H
