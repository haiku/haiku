/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "LockTestSuite.h"

#include "RWLockTests.h"


TestSuite*
create_lock_test_suite()
{
	TestSuite* suite = new(std::nothrow) TestSuite("lock");

	ADD_TEST(suite, create_rw_lock_test_suite());

	return suite;
}
