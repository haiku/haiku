/*
 * Copyright 2021 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXCLUSIVE_BORROW_TEST_H
#define EXCLUSIVE_BORROW_TEST_H

#include <TestCase.h>
#include <TestSuite.h>


class ExclusiveBorrowTest : public BTestCase
{
public:
								ExclusiveBorrowTest();

			void				ObjectDeleteTest();
			void				OwnershipTest();
			void				PolymorphismTest();
			void				ReleaseTest();

	static	void				AddTests(BTestSuite& suite);
};


#endif // EXCLUSIVE_BORROW_TEST_H
