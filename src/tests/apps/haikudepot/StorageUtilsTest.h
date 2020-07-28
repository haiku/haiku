/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef STORAGE_UTILS_TEST_H
#define STORAGE_UTILS_TEST_H

#include <TestCase.h>
#include <TestSuite.h>


class StorageUtilsTest : public CppUnit::TestCase {
public:
								StorageUtilsTest();
	virtual						~StorageUtilsTest();

			void				TestSwapExtensionOnPathComponent();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// STORAGE_UTILS_TEST_H
