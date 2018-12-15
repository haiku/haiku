/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef LIST_TEST_H
#define LIST_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class ListTest : public CppUnit::TestCase {
public:
								ListTest();
	virtual						~ListTest();

			void				TestAddOrdered();
			void				TestBinarySearch();
			void				TestAddUnordered();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// LIST_TEST_H
