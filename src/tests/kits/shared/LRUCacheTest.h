/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef LRU_CACHE_TEST_H
#define LRU_CACHE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class LRUCacheTest : public CppUnit::TestCase {
public:
								LRUCacheTest();
	virtual						~LRUCacheTest();

			void				TestAddWithOverflow();
			void				TestAddWithOverflowWithGets();
			void				TestRemove();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// LRU_CACHE_TEST_H
