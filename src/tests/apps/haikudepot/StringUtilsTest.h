/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_UTILS_TEST_H
#define STRING_UTILS_TEST_H

#include <TestCase.h>
#include <TestSuite.h>


class StringUtilsTest : public CppUnit::TestCase {
public:
								StringUtilsTest();
	virtual						~StringUtilsTest();

			void				TestStartInSituTrimSpaceAndControl();
			void				TestEndInSituTrimSpaceAndControl();
			void				TestStartAndEndInSituTrimSpaceAndControl();
			void				TestNoTrimInSituTrimSpaceAndControl();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// STRING_UTILS_TEST_H
